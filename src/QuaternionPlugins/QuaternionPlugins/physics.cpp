#include "stdafx.h"
#include "MathExtension.h"
#include "btBulletDynamicsCommon.h"
#include "btCasts.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#include <cmath>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <new>
#include <algorithm>

#define ADJACENCY_NORMAL_THRESHOLD 0.1f

extern ContactStartedCallback gContactStartedCallback;
extern ContactAddedCallback gContactAddedCallback;
extern ContactProcessedCallback gContactProcessedCallback;

int numCalls;

btDiscreteDynamicsWorld* dynamicsWorld;

struct ShapeInfo {
	btCollisionShape *shape;
	std::vector<btVector3*> triangleData;
	std::vector<std::pair<int, int>> adjacent;
};

struct BodyInfo {
	btRigidBody *body;
	ShapeInfo shape;
	btVector3 collisionNormal;
	btVector3 collisionNormalTemp;
	std::vector<btVector3> compressingImpulses;
	float compression;
	bool isDynamic;
	bool isCallback;
	bool isPlayer;
	bool defunct;
	std::vector<int> collidingIndices;
	float timeout;
};

class BodyMovement {
private:
	btRigidBody *body;
public:
	virtual bool updateTransform(float timeDelta) = 0;
	btRigidBody *getBody() const { return this->body; }
	void setBody(btRigidBody *body) { this->body = body; }
	virtual btVector3 getVelocity() const = 0;
	virtual void setVelocity(const btVector3 &velocity) = 0;
	virtual btVector3 getAngularVelocity() const = 0;
	virtual void setAngularVelocity(const btVector3 &velocity) = 0;

	// We need to overload these so that VC++ doesn't yell at us (and so the game doesn't crash).
	// Bullet uses some 16-bit alignments, so we need to malloc with 16-bit alignment.

	void* operator new (size_t size) {
		void* ptr = _aligned_malloc(size, 16);
		if (!ptr)
			throw std::bad_alloc();
		return ptr;
	}

	void operator delete (void *ptr) {
		_aligned_free(ptr);
	}

	void* operator new[] (size_t size) {
		void* ptr = _aligned_malloc(size, 16);
		if (!ptr)
			throw std::bad_alloc();
		return ptr;
	}

	void operator delete[] (void *ptr) {
		_aligned_free(ptr);
	}
};

class VelocityMovement : public BodyMovement {
private:
	btVector3 velocity = btVector3(0.f, 0.f, 0.f);
	btVector3 angularVelocity = btVector3(0.f, 0.f, 0.f);
public:
	VelocityMovement(btRigidBody *body) { this->setBody(body); }
	bool updateTransform(float timeDelta) {
		btTransform trans;
		this->getBody()->getMotionState()->getWorldTransform(trans);
		btVector3 pos = trans.getOrigin();
		btQuaternion rot = trans.getRotation();
		btVector3 newpos = pos + timeDelta*this->velocity;
		btVector3 axis = this->angularVelocity;
		axis.safeNormalize();
		btScalar angle = timeDelta*this->angularVelocity.length();
		btQuaternion newrot = btQuaternion(axis, angle) * rot;
		trans.setOrigin(newpos);
		trans.setRotation(newrot);
		this->getBody()->getMotionState()->setWorldTransform(trans);
		this->getBody()->setLinearVelocity(velocity);
		this->getBody()->setAngularVelocity(angularVelocity);
		return false;
	}
	btVector3 getVelocity() const { return this->velocity; }
	void setVelocity(const btVector3 &velocity) { this->velocity = velocity; }
	btVector3 getAngularVelocity() const { return this->angularVelocity; }
	void setAngularVelocity(const btVector3 &velocity) { this->angularVelocity = velocity; }
};

class ParentMovement : public BodyMovement {
private:
	btRigidBody *parent;
	btTransform offset;
	
	btVector3 cachedVelocity;
	btVector3 cachedAngularVelocity;

public:
	ParentMovement(btRigidBody *body, btRigidBody *parent, const btTransform &offset) : parent(parent), offset(offset) {
		this->setBody(body);
		this->cachedVelocity = btVector3(0.f, 0.f, 0.f);
		this->cachedAngularVelocity = btVector3(0.f, 0.f, 0.f);
	}
	bool updateTransform(float timeDelta) {
		btTransform trans;
		trans = this->parent->getWorldTransform();
		trans *= this->offset;
		
		btTransform oldTrans;
		this->getBody()->getMotionState()->getWorldTransform(oldTrans);

		this->getBody()->getMotionState()->setWorldTransform(trans);

		btVector3 newPos = trans.getOrigin();
		btVector3 oldPos = oldTrans.getOrigin();
		btVector3 velocity = (newPos - oldPos)*(1.f / timeDelta);

		btQuaternion newRot = trans.getRotation();
		btQuaternion oldRot = oldTrans.getRotation();
		btQuaternion invOld = oldRot.inverse();
		btQuaternion diffQuat = newRot * invOld;
		btVector3 angularVelocity = diffQuat.getAxis() * diffQuat.getAngle() / timeDelta;

		this->cachedVelocity = velocity;
		this->cachedAngularVelocity = angularVelocity;

		this->getBody()->setLinearVelocity(velocity);
		this->getBody()->setAngularVelocity(angularVelocity);

		return false;
	}
	btVector3 getVelocity() const { return this->cachedVelocity; }
	void setVelocity(const btVector3 &velocity) { TGE::Con::warnf("Velocity setting is not supported for this movement type."); }
	btVector3 getAngularVelocity() const { return this->cachedAngularVelocity; }
	void setAngularVelocity(const btVector3 &velocity) { TGE::Con::warnf("Velocity setting is not supported for this movement type."); }
};

class PathedMovement : public BodyMovement {
private:
	TGE::SimGroup *pathGroup;
	btTransform offset;
	btVector3 cachedVelocity;
	btVector3 cachedAngularVelocity;
	float t;
	float totalT;
	int targetMs;
	std::string twoPrevNode;
	std::string prevNode;
	std::string nextNode;
	std::string twoNextNode;
	std::string findNode(U32 ind, bool tryAgain) const {
		U32 maxInd = 0;
		for (U32 i = 0; i < (U32)this->pathGroup->objectList.size(); i++) {
			TGE::SimObject *obj = this->pathGroup->objectList.at(i);
			const char *seqNum = getObjectField(obj, "seqNum");
			if (seqNum[0]) {
				U32 num = atoi(seqNum);
				if (num == ind)
					return std::string(obj->getIdString());
				if (num > maxInd)
					maxInd = num;
			}
		}
		if (tryAgain) {
			int rem = ind % (maxInd + 1);
			if (rem < 0)
				rem += maxInd + 1;
			return this->findNode(rem, false);
		}
		return "";
	}
	std::string findNode(U32 ind) const { return this->findNode(ind, true); }
	void updateNodes() {
		if (TGE::Sim::findObject(this->prevNode.c_str()) == NULL)
			return;
		const char* msString = getObjectField(TGE::Sim::findObject(this->prevNode.c_str()), "msToNext");
		float msToNext = (float)atof(msString);
		while (this->t > msToNext / 1000.f) {
			this->t -= msToNext / 1000.f;
			this->prevNode = this->nextNode;
			int seqNum = atoi(getObjectField(TGE::Sim::findObject(this->prevNode.c_str()), "seqNum"));
			this->nextNode = this->findNode(seqNum + 1);
			this->twoPrevNode = this->findNode(seqNum - 1);
			this->twoNextNode = this->findNode(seqNum + 2);
			msString = getObjectField(TGE::Sim::findObject(this->prevNode.c_str()), "msToNext");
			msToNext = (float)atof(msString);
		}
		while (this->t < 0.f) {
			this->prevNode = this->twoPrevNode;
			int seqNum = atoi(getObjectField(TGE::Sim::findObject(this->prevNode.c_str()), "seqNum"));
			this->nextNode = this->findNode(seqNum + 1);
			this->twoPrevNode = this->findNode(seqNum - 1);
			this->twoNextNode = this->findNode(seqNum + 2);
			msString = getObjectField(TGE::Sim::findObject(this->prevNode.c_str()), "msToNext");
			msToNext = (float)atof(msString);
			this->t += msToNext / 1000.f;
		}

	}
	Point3F bezier(Point3F p1, Point3F p2, Point3F p3, Point3F p4, float t) const {
		return p1 * pow(1.f - t, 3) + p2 * pow(1.f - t, 2) * t + p3 * (1.f - t) * pow(t, 2) + p4 * pow(t, 3);
	}
	Point3F bezierVel(Point3F p1, Point3F p2, Point3F p3, Point3F p4, float t) const {
		return (p2 - p1) * 3.f * pow(1.f - t, 2) + (p3 - p2) * 6.f * (1.f - t) * t + (p4 - p3) * 3.f * pow(t, 2);
	}
	QuatF rotBezier(float t, std::vector<QuatF> vec, int begin, int end) const {
		if (end == begin + 1)
			return vec[begin];

		QuatF b1 = this->rotBezier(t, vec, begin, end - 1);
		QuatF b2 = this->rotBezier(t, vec, begin + 1, end);
		b1.interpolate(b1, b2, t);
		return b1;
	}
	QuatF rotCubic(float t1, float t2, float t3, float t4, QuatF r1, QuatF r2, QuatF r3, QuatF r4, float t) const {
		QuatF d2 = r3;
		d2 /= r1;
		d2 /= (t3 - t1);
		QuatF d3 = r4;
		d3 /= r2;
		d3 /= (t4 - t2);
		QuatF rd2 = d2 * ((t3 - t2) / 3.f);
		rd2 *= r2;
		QuatF rd3 = d3 * ((t2 - t3) / 3.f);
		rd3 *= r3;
		std::vector<QuatF> vec = std::vector<QuatF>();
		vec.push_back(r2);
		vec.push_back(rd2);
		vec.push_back(rd3);
		vec.push_back(r3);
		return this->rotBezier((t - t2) / (t3 - t2), vec, 0, 4);
	}
public:
	PathedMovement(btRigidBody *body, TGE::SimGroup *path, const btTransform &offset) : pathGroup(path), offset(offset),
			t(0.f), totalT(0.f), targetMs(-1) {
		this->setBody(body);
		this->twoPrevNode = "";
		this->prevNode = this->findNode(0);
		this->nextNode = this->findNode(1);
		this->twoNextNode = this->findNode(2);

		this->cachedVelocity = btVector3(0.f, 0.f, 0.f);
		this->cachedAngularVelocity = btVector3(0.f, 0.f, 0.f);
	}
	bool updateTransform(float timeDelta) {
		if (this->targetMs < 0) {
			this->t += timeDelta;
			this->totalT += timeDelta;
		}
		else if (this->totalT + timeDelta < ((float)this->targetMs) / 1000.f) {
			this->t += timeDelta;
			this->totalT += timeDelta;
		}
		else if (this->totalT - timeDelta > ((float)this->targetMs) / 1000.f) {
			this->t -= timeDelta;
			this->totalT -= timeDelta;
		}
		else {
			this->t += ((float)this->targetMs) / 1000.f - this->totalT;
			this->totalT = ((float)this->targetMs) / 1000.f;
		}
		this->updateNodes();
		TGE::SceneObject *prevSO = reinterpret_cast<TGE::SceneObject*>(TGE::Sim::findObject(this->prevNode.c_str()));
		TGE::SceneObject *nextSO = reinterpret_cast<TGE::SceneObject*>(TGE::Sim::findObject(this->nextNode.c_str()));
		if (prevSO == NULL)
			return true;
		if (nextSO == NULL)
			return true;
		MatrixF prevT = prevSO->getTransform();
		MatrixF nextT = nextSO->getTransform();
		Point3F prev = prevT.getPosition();
		Point3F next = nextT.getPosition();
		QuatF prevRot = QuatF(prevT);
		QuatF nextRot = QuatF(nextT);
		float dist = (next - prev).len();
		Point3F a = prev + prevT.getForwardVector() * (dist / 3.f);
		Point3F b = next + nextT.getForwardVector() * (dist / 3.f);
		float msToNext = (float)atof(getObjectField(prevSO, "msToNext")) / 1000.f;
		float tm = this->t / msToNext;
		const char* type = getObjectField(prevSO, "smoothingType");
		const char* rotType = getObjectField(prevSO, "rotationType");
		if (!rotType[0])
			rotType = type;

		Point3F pos;
		QuatF rot;

		if (_stricmp(type, "Spline")) {
			float tm2;
			if (!_stricmp(type, "Accelerate"))
				tm2 = (tm <= 0.5f) ? (tm*tm*2.f) : 1 - ((1 - tm)*(1 - tm)*2.f);
			else
				tm2 = tm;
			pos.interpolate(prev, next, tm2);
		}
		else {
			pos = this->bezier(prev, a, b, next, tm);
		}

		if (_stricmp(rotType, "Spline") && _stricmp(rotType, "Cubic")) {
			float tm2;
			if (!_stricmp(rotType, "Accelerate"))
				tm2 = (tm <= 0.5f) ? (tm*tm*2.f) : 1 - ((1 - tm)*(1 - tm)*2.f);
			else
				tm2 = tm;
			rot.interpolate(prevRot, nextRot, tm2);
		}
		else if (!_stricmp(rotType, "Cubic")) {
			TGE::SceneObject *twoPrevSO = reinterpret_cast<TGE::SceneObject*>(TGE::Sim::findObject(this->twoPrevNode.c_str()));
			TGE::SceneObject *twoNextSO = reinterpret_cast<TGE::SceneObject*>(TGE::Sim::findObject(this->twoNextNode.c_str()));
			if (twoPrevSO == NULL)
				twoPrevSO = prevSO;
			if (twoNextSO == NULL)
				twoNextSO = nextSO;

			QuatF twoPrevRot = QuatF(twoPrevSO->getTransform());
			QuatF twoNextRot = QuatF(twoNextSO->getTransform());

			float twoPrevTime = -(float)atof(getObjectField(twoPrevSO, "msToNext")) / 1000.f;
			float prevTime = 0.f;
			float nextTime = msToNext;
			float twoNextTime = (float)atof(getObjectField(nextSO, "msToNext")) / 1000.f;

			rot = this->rotCubic(twoPrevTime, prevTime, nextTime, twoNextTime, twoPrevRot, prevRot, nextRot, twoNextRot, this->t);
		}
		else {
			rot.interpolate(prevRot, nextRot, tm);
			Point3F point = this->bezierVel(prev, a, b, next, tm);
			Point3F to = Point3F(0.f, 1.f, 0.f);
			MatrixF mat = MatrixF();
			rot.setMatrix(&mat);
			mat.mulP(to, &to);
			QuatF vecrot = QuatF(VectorRot(to, point));
			vecrot *= rot;
			rot = vecrot;
		}

		MatrixF newmat = MatrixF();
		rot.setMatrix(&newmat);
		newmat.setPosition(pos);
		newmat *= btCast<MatrixF>(this->offset);

		btTransform oldTrans;
		this->getBody()->getMotionState()->getWorldTransform(oldTrans);

		btTransform newTrans = btCast<btTransform>(newmat);
		this->getBody()->getMotionState()->setWorldTransform(newTrans);

		btVector3 newPos = newTrans.getOrigin();
		btVector3 oldPos = oldTrans.getOrigin();
		btVector3 velocity = (newPos - oldPos)*(1.f / timeDelta);

		btQuaternion newRot = newTrans.getRotation();
		btQuaternion oldRot = oldTrans.getRotation();
		btQuaternion invOld = oldRot.inverse();
		btQuaternion diffQuat = newRot * invOld;
		btVector3 angularVelocity = diffQuat.getAxis() * diffQuat.getAngle() / timeDelta;

		this->cachedVelocity = velocity;
		this->cachedAngularVelocity = angularVelocity;

		this->getBody()->setLinearVelocity(velocity);
		this->getBody()->setAngularVelocity(angularVelocity);

		return false;
	}
	btVector3 getVelocity() const { return this->cachedVelocity; }
	void setVelocity(const btVector3 &velocity) { TGE::Con::warnf("Velocity setting is not supported for this movement type."); }
	btVector3 getAngularVelocity() const { return this->cachedAngularVelocity; }
	void setAngularVelocity(const btVector3 &velocity) { TGE::Con::warnf("Velocity setting is not supported for this movement type."); }

	void setTargetTimeMS(int targetTime) { this->targetMs = targetTime; }
	float getPathTimeMS() { return this->totalT * 1000.0f; }
};

class ApproachMovement : public BodyMovement {
private:
	F32 speed = 1;
	F32 rotSpeed = 1;
	btTransform target = btTransform();
	VelocityMovement decorated = VelocityMovement(NULL);
public:
	ApproachMovement(btRigidBody *body, F32 sp, F32 rotSp) : speed(sp), rotSpeed(rotSp) {
		this->setBody(body);
		this->decorated = VelocityMovement(body);
		this->getBody()->getMotionState()->getWorldTransform(this->target);
	}
	bool updateTransform(float timeDelta) {
		btTransform trans;
		this->getBody()->getMotionState()->getWorldTransform(trans);

		btVector3 startPos = trans.getOrigin();
		btVector3 endPos = target.getOrigin();
		btScalar distance = (endPos - startPos).length();
		if (distance > 10000)
			return false;

		if (distance >= this->speed*timeDelta) {
			btVector3 velocity = (endPos - startPos).safeNormalize() * this->speed;
			this->decorated.setVelocity(velocity);
		}
		else {
			this->decorated.setVelocity(btVector3(0.f, 0.f, 0.f));
			trans.setOrigin(endPos);
			this->getBody()->getMotionState()->setWorldTransform(trans);
		}

		btQuaternion startRot = trans.getRotation();
		btQuaternion endRot = target.getRotation();
		btQuaternion invStart = startRot.inverse();
		btQuaternion diffQuat = endRot * invStart;
		btScalar distanceRot = diffQuat.getAngle();

		if (distanceRot >= this->rotSpeed*timeDelta) {
			btVector3 angularVelocity = diffQuat.getAxis() * this->rotSpeed;
			this->decorated.setAngularVelocity(angularVelocity);
		}
		else {
			this->decorated.setAngularVelocity(btVector3(0.f, 0.f, 0.f));
			trans.setRotation(endRot);
			this->getBody()->getMotionState()->setWorldTransform(trans);
		}

		return this->decorated.updateTransform(timeDelta);
	}
	btVector3 getVelocity() const { return this->decorated.getVelocity(); }
	void setVelocity(const btVector3 &velocity) { TGE::Con::warnf("Velocity setting is not supported for this movement type."); }
	btVector3 getAngularVelocity() const { return this->decorated.getAngularVelocity(); }
	void setAngularVelocity(const btVector3 &velocity) { TGE::Con::warnf("Velocity setting is not supported for this movement type."); }

	void setTargetTransform(const btTransform &targetTransform) {
		this->target = targetTransform;
	}

	btTransform getTargetTransform() {
		return this->target;
	}
};

std::vector<ShapeInfo> shapes;
std::vector<BodyInfo> bodies;
std::vector<BodyMovement*> moves;

void tickCallback(btDynamicsWorld *world, btScalar timeStep) {

	for (U32 i = 0; i < bodies.size(); i++) {
		bodies[i].collisionNormalTemp = btVector3(0.0f, 0.0f, 0.0f);
		bodies[i].compressingImpulses.clear();
	}

	btDispatcher *dp = dynamicsWorld->getDispatcher();
	U32 numManifolds = dp->getNumManifolds();
	for (U32 i = 0; i < numManifolds; i++) {
		btPersistentManifold *contactManifold = dp->getManifoldByIndexInternal(i);
		const btCollisionObject *objA = contactManifold->getBody0();
		const btCollisionObject *objB = contactManifold->getBody1();

		if (objA->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
			continue;
		if (objB->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE)
			continue;

		int indA = objA->getUserIndex(), indB = objB->getUserIndex();

		U32 numContacts = contactManifold->getNumContacts();
		for (U32 j = 0; j < numContacts; j++) {
			btManifoldPoint &pt = contactManifold->getContactPoint(j);
			if (pt.getDistance() < 0.001f) {
				const btVector3 &normalOnB = pt.m_normalWorldOnB;
				if (indA >= 0)
					bodies[indA].collisionNormalTemp += normalOnB;
				if (indB >= 0)
					bodies[indB].collisionNormalTemp -= normalOnB;

				// Compute inward/outward impulse and push onto vector
				bodies[indA].compressingImpulses.push_back(normalOnB * pt.m_appliedImpulse);
				bodies[indB].compressingImpulses.push_back(-normalOnB * pt.m_appliedImpulse);
			}
		}

		//contactManifold->clearManifold();
	}

	for (U32 i = 0; i < bodies.size(); i++) {
		if (bodies[i].collisionNormalTemp.length() > 0.5f)
			bodies[i].collisionNormalTemp.normalize();

		bodies[i].timeout -= 1000.0f*timeStep;
		if (bodies[i].collisionNormalTemp.length() > 0.01f || bodies[i].timeout <= 0.f) {
			bodies[i].timeout = 75.f;
			bodies[i].collisionNormal = bodies[i].collisionNormalTemp;
		}

		bodies[i].compression = 0.f;

		if (bodies[i].compressingImpulses.size() == 0)
			continue;

		btVector3 totalImpulse(0.f, 0.f, 0.f);
		
		// screw c++ iterators, can't be bothered to remember how to do them atm
		for (U32 j = 0; j < bodies[i].compressingImpulses.size(); j++)
			totalImpulse += bodies[i].compressingImpulses[j];

		btVector3 averageImpulse = totalImpulse / (bodies[i].compressingImpulses.size());
		if (averageImpulse.length() < 0.001f)
			continue;

		float totalCompression = 0.f;
		for (U32 j = 0; j < bodies[i].compressingImpulses.size(); j++)
			totalCompression += (bodies[i].compressingImpulses[j] - averageImpulse).length2();

		bodies[i].compression = mSqrt(totalCompression) / averageImpulse.length();

	}

	for (U32 i = 0; i < moves.size(); i++) {
		if (moves[i]->updateTransform(timeStep)) {
			moves.erase(moves.begin() + i);
			i--;
		}
	}
}

template <typename T>
bool vectorContains(const std::vector<T>& v, T x) {
	return std::find(v.begin(), v.end(), x) != v.end();
}

void customContactCallback(btPersistentManifold* const &manifold) {
	btBvhTriangleMeshShape *trimesh0 = dynamic_cast<btBvhTriangleMeshShape*>(const_cast<btCollisionShape*>(manifold->getBody0()->getCollisionShape()));
	btBvhTriangleMeshShape *trimesh1 = dynamic_cast<btBvhTriangleMeshShape*>(const_cast<btCollisionShape*>(manifold->getBody1()->getCollisionShape()));
	btSphereShape *sphere0 = dynamic_cast<btSphereShape*>(const_cast<btCollisionShape*>(manifold->getBody0()->getCollisionShape()));
	btSphereShape *sphere1 = dynamic_cast<btSphereShape*>(const_cast<btCollisionShape*>(manifold->getBody1()->getCollisionShape()));
	btBvhTriangleMeshShape *trimesh;
	const btCollisionObject *body, *otherBody;

	const btCollisionObject *body0, *body1;
	body0 = manifold->getBody0();
	body1 = manifold->getBody1();

	if (bodies[body0->getUserIndex()].isCallback && !vectorContains(bodies[body0->getUserIndex()].collidingIndices, body1->getUserIndex())) {
		std::ostringstream varname;
		varname << "$btGlobalBodyIndex";
		varname << body1->getUserIndex();
		std::ostringstream varname2;
		varname2 << "$btGlobalBodyIndex";
		varname2 << body0->getUserIndex();
		const char* tgeobj = TGE::Con::getVariable(varname.str().c_str());
		const char* tgecol = TGE::Con::getVariable(varname2.str().c_str());
		TGE::Con::evaluatef("(%s).getDataBlock().onCollision(%s, %s);", tgeobj, tgeobj, tgecol);
		// TGE::Con::printf("Body %d collided with body %d", body0->getUserIndex(), body1->getUserIndex());
		bodies[body0->getUserIndex()].collidingIndices.push_back(body1->getUserIndex());
	}
	if (bodies[body1->getUserIndex()].isCallback && !vectorContains(bodies[body1->getUserIndex()].collidingIndices, body0->getUserIndex())) {
		std::ostringstream varname;
		varname << "$btGlobalBodyIndex";
		varname << body0->getUserIndex();
		std::ostringstream varname2;
		varname2 << "$btGlobalBodyIndex";
		varname2 << body1->getUserIndex();
		const char* tgeobj = TGE::Con::getVariable(varname.str().c_str());
		const char* tgecol = TGE::Con::getVariable(varname2.str().c_str());
		TGE::Con::evaluatef("(%s).getDataBlock().onCollision(%s, %s);", tgeobj, tgeobj, tgecol);
		// TGE::Con::printf("Body %d collided with body %d", body1->getUserIndex(), body0->getUserIndex());
		bodies[body1->getUserIndex()].collidingIndices.push_back(body0->getUserIndex());
	}

	// return; // Don't do any of the below stuff, it's glitchy as heck!!

	if (trimesh0 != NULL && sphere1 != NULL) {
		trimesh = trimesh0;
		body = manifold->getBody0();
		otherBody = manifold->getBody1();
	}
	else if (trimesh1 != NULL && sphere0 != NULL) {
		trimesh = trimesh1;
		body = manifold->getBody1();
		otherBody = manifold->getBody0();
	}
	else
		return;

	int ind = body->getUserIndex();
	if (ind == -1)
		return;

	BodyInfo *bodyInfo = &bodies[ind];

	int ind2 = otherBody->getUserIndex();
	if (ind2 == -1)
		return;

	BodyInfo *otherBodyInfo = &bodies[ind2];

	// For now, only enable filtering for the player
	if (!otherBodyInfo->isPlayer)
		return;

	btTriangleMesh *mesh_int = (btTriangleMesh*)trimesh->getMeshInterface();

	numCalls++;

	const int filterLimit = 10; // Limit on filtering for speed
	if (numCalls > filterLimit)
		return;

	// TGE::Con::printf("Collision filtering for body %d", ind2);

	std::vector<int> triangleIndices;
	bool removed = false;

	for (int i = 0; i < manifold->getNumContacts(); i++) {
		int index;
		if (trimesh0 != NULL)
			index = manifold->getContactPoint(i).m_index0;
		else
			index = manifold->getContactPoint(i).m_index1;

		for (U32 j = 0; j < triangleIndices.size(); j++) {
			// Figure out if they're adjacent!
			btVector3 *points = new btVector3[3];
			points = bodyInfo->shape.triangleData[index];
			btVector3 *others = new btVector3[3];
			others = bodyInfo->shape.triangleData[triangleIndices[j]];

			// Count number of matched vertices
			int matches = 0;
			if (points[0] == others[0] || points[0] == others[1] || points[0] == others[2])
				matches++;
			if (points[1] == others[0] || points[1] == others[1] || points[1] == others[2])
				matches++;
			if (points[2] == others[0] || points[2] == others[1] || points[2] == others[2])
				matches++;

			// Calculate normals
			btVector3 normal1 = (points[1] - points[0]).cross(points[2] - points[0]);
			btVector3 normal2 = (others[1] - others[0]).cross(others[2] - others[0]);
			normal1.safeNormalize();
			normal2.safeNormalize();
			btVector3 cross = normal1.cross(normal2);

			if (cross.length() < sin(ADJACENCY_NORMAL_THRESHOLD) && matches >= 1) {
				if (manifold->getContactPoint(i).getDistance() < manifold->getContactPoint(j).getDistance()) {
					int index;
					if (trimesh0 != NULL)
						index = manifold->getContactPoint(i).m_index0;
					else
						index = manifold->getContactPoint(i).m_index1;
					btVector3 *points;
					points = bodyInfo->shape.triangleData[index];
					btVector3 normal = (points[1] - points[0]).cross(points[2] - points[0]);
					normal.safeNormalize();

					btQuaternion rot = bodyInfo->body->getWorldTransform().getRotation();
					normal = normal.rotate(rot.getAxis(), rot.getAngle());

					if (trimesh0 != NULL)
						normal *= -1;

					if (normal.dot(manifold->getContactPoint(i).m_normalWorldOnB) < 0)
						normal *= -1;

					manifold->getContactPoint(i).m_normalWorldOnB = normal;
					manifold->removeContactPoint(j);
				}
				else {
					int index;
					if (trimesh0 != NULL)
						index = manifold->getContactPoint(j).m_index0;
					else
						index = manifold->getContactPoint(j).m_index1;
					btVector3 *points;
					points = bodyInfo->shape.triangleData[index];
					btVector3 normal = (points[1] - points[0]).cross(points[2] - points[0]);
					normal.safeNormalize();

					btQuaternion rot = bodyInfo->body->getWorldTransform().getRotation();
					normal = normal.rotate(rot.getAxis(), rot.getAngle());

					if (trimesh0 != NULL)
						normal *= -1;

					if (normal.dot(manifold->getContactPoint(j).m_normalWorldOnB) < 0)
						normal *= -1;

					manifold->getContactPoint(j).m_normalWorldOnB = normal;
					manifold->removeContactPoint(i);
				}
				// TGE::Con::errorf("Point removed");
				removed = true;
				break;
			}
		}

		//if (!removed)
			//TGE::Con::errorf("%f %f %f %f", manifold->getContactPoint(i).m_normalWorldOnB.x(), manifold->getContactPoint(i).m_normalWorldOnB.y(), manifold->getContactPoint(i).m_normalWorldOnB.z(), manifold->getContactPoint(i).);

		triangleIndices.push_back(index);
	}

	if (false) {
		for (int i = 0; i < manifold->getNumContacts(); i++) {
			int index;
			if (trimesh0 != NULL)
				index = manifold->getContactPoint(i).m_index0;
			else
				index = manifold->getContactPoint(i).m_index1;
			btVector3 *points;
			points = bodyInfo->shape.triangleData[index];
			btVector3 normal = (points[1] - points[0]).cross(points[2] - points[0]);
			normal.safeNormalize();

			btQuaternion rot = bodyInfo->body->getWorldTransform().getRotation();
			normal = normal.rotate(rot.getAxis(), rot.getAngle());

			if (trimesh0 != NULL)
				normal *= -1;

			if (normal.dot(manifold->getContactPoint(i).m_normalWorldOnB) < 0)
				normal *= -1;
			else
				TGE::Con::errorf("hello %f %f %f; %f %f %f", normal.x(), normal.y(), normal.z(), manifold->getContactPoint(i).m_normalWorldOnB.x(), manifold->getContactPoint(i).m_normalWorldOnB.y(), manifold->getContactPoint(i).m_normalWorldOnB.z());

			// FIX: make sure the normal makes an obtuse angle with the vector from the other body's center
			// Currently seems ineffective
			/*btVector3 diff;
			if (trimesh0 != NULL)
				diff = manifold->getContactPoint(i).getPositionWorldOnB() - ((btRigidBody*)(manifold->getBody1()))->getWorldTransform().getOrigin();
			else
				diff = manifold->getContactPoint(i).getPositionWorldOnA() - ((btRigidBody*)(manifold->getBody0()))->getWorldTransform().getOrigin();

			if (diff.dot(normal) > 0)
				normal *= -1;*/

			manifold->getContactPoint(i).m_normalWorldOnB = normal;
		}
	}
}

bool customContactAddedCallback(btManifoldPoint& cp, const btCollisionObjectWrapper *colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper *colObj1Wrap, int partId1, int index1) {
	btAdjustInternalEdgeContacts(cp, colObj0Wrap, colObj1Wrap, partId1, index1, BT_TRIANGLE_CONVEX_DOUBLE_SIDED);
	btAdjustInternalEdgeContacts(cp, colObj1Wrap, colObj0Wrap, partId0, index0, BT_TRIANGLE_CONVEX_DOUBLE_SIDED);
	return true;
}

bool customContactProcessedCallback(btManifoldPoint& cp, void *colObj0P, void *colObj1P) {
	btCollisionObject *colObj0 = static_cast<btCollisionObject*>(colObj0P);
	btCollisionObject *colObj1 = static_cast<btCollisionObject*>(colObj1P);

	return true;
}

void initBullet() {

	gContactStartedCallback = customContactCallback;
	gContactAddedCallback = customContactAddedCallback;
	gContactProcessedCallback = customContactProcessedCallback;
	btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
	btCollisionDispatcher* dispatcher = new	btCollisionDispatcher(collisionConfiguration);
	btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setInternalTickCallback(tickCallback);
	dynamicsWorld->setGravity(btVector3(0, 0, -20));

	// makeGroundShape();

}

void updateBullet(U32 timeDelta) {
	// TGE::Con::errorf("NEXT STEP");

	numCalls = 0;
	for (std::vector<BodyInfo>::iterator it = bodies.begin(); it != bodies.end(); it++) {
		(*it).collidingIndices.clear();
	}

	dynamicsWorld->stepSimulation(((float)timeDelta)/1000.0f, 10);
}

ConsoleFunction(btResetWorld, void, 1, 1, "btResetWorld()") {
	for (U32 i = 0; i < bodies.size(); i++) {
		bodies[i].defunct = true;
		dynamicsWorld->removeRigidBody(bodies[i].body);
	}

	moves.clear();

	// makeGroundShape();
}

ConsoleFunction(btCreateShapeBox, int, 2, 2, "btCreateShapeBox( halfExtent )") {
	btCollisionShape* colShape = new btBoxShape(btCast<btVector3>(StringMath::scan<Point3F>(argv[1])));
	ShapeInfo info;
	info.shape = colShape;
	shapes.push_back(info);
	return shapes.size() - 1;
}

ConsoleFunction(btCreateShapeSphere, int, 2, 2, "btCreateShapeSphere( radius )") {
	btCollisionShape* colShape = new btSphereShape((float)atof(argv[1]));
	ShapeInfo info;
	info.shape = colShape;
	shapes.push_back(info);
	return shapes.size() - 1;
}

ConsoleFunction(btCreateShapeRAW, int, 2, 2, "btCreateShapeRAW( file )") {
	char path[256];
	TGE::Con::expandScriptFilename(path, 256, argv[1]);
	std::string fullPath = std::string(TGE::Platform::getWorkingDirectory());
	fullPath += '/';
	fullPath += path;

	std::ifstream ifs(fullPath, std::ifstream::in);
	btTriangleMesh *mesh = new btTriangleMesh();
	float x1, y1, z1, x2, y2, z2, x3, y3, z3;
	std::vector<btVector3*> triData;
	std::vector<std::pair<int, int>> adjacent;
	int i = 0;

	while (ifs >> x1 >> y1 >> z1 >> x2 >> y2 >> z2 >> x3 >> y3 >> z3) {
		btVector3 v1(x1, y1, z1);
		btVector3 v2(x2, y2, z2);
		btVector3 v3(x3, y3, z3);

		mesh->addTriangle(v1, v2, v3);
		btVector3 *points = new btVector3[3];
		points[0] = v1;
		points[1] = v2;
		points[2] = v3;

		/*for (U32 j = 0; j < triData.size(); j++) {
			// Figure out if they're adjacent!
			btVector3 *others = new btVector3[3];
			others = triData[j];

			// Count number of matched vertices
			int matches = 0;
			if (points[0] == others[0] || points[0] == others[1] || points[0] == others[2])
				matches++;
			if (points[1] == others[0] || points[1] == others[1] || points[1] == others[2])
				matches++;
			if (points[2] == others[0] || points[2] == others[1] || points[2] == others[2])
				matches++;

			// Calculate normals
			btVector3 normal1 = (points[1] - points[0]).cross(points[2] - points[0]);
			btVector3 normal2 = (others[1] - others[0]).cross(others[2] - others[0]);
			normal1.safeNormalize();
			normal2.safeNormalize();
			btVector3 cross = normal1.cross(normal2);
			
			if (cross.length() < sin(ADJACENCY_NORMAL_THRESHOLD) && matches >= 1) {
				adjacent.push_back(std::make_pair(i, j));
				adjacent.push_back(std::make_pair(j, i));
			}
		}*/

		triData.push_back(points);
		i++;
	}
	ifs.close();

	btCollisionShape* colShape = new btBvhTriangleMeshShape(mesh, true);
	ShapeInfo info;
	info.shape = colShape;
	info.triangleData = triData;
	info.adjacent = adjacent;
	shapes.push_back(info);
	return shapes.size() - 1;
}

ConsoleFunction(btCreateBody, int, 4, 4, "btCreateBody( shape, mass, transform )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= shapes.size()) {
		TGE::Con::errorf("Invalid shape handle!");
		return -1;
	}
	btCollisionShape* colShape = shapes[ind].shape;

	/// Create Dynamic Objects
	btTransform startTransform = btCast<btTransform>(StringMath::scan<MatrixF>(argv[3]));

	btScalar	mass((float)atof(argv[2]));

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.f);

	if (isDynamic && dynamic_cast<btBvhTriangleMeshShape*>(colShape) != NULL) {
		TGE::Con::errorf("Dynamic objects cannot use RAW meshes!");
		return -1;
	}

	btVector3 localInertia(0, 0, 0);
	if (isDynamic) {
		colShape->calculateLocalInertia(mass, localInertia);
	}

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	body->setActivationState(DISABLE_DEACTIVATION);
	if (!isDynamic)
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);


	dynamicsWorld->addRigidBody(body);
	
	BodyInfo info;
	info.body = body;
	info.collisionNormal = btVector3(0.0f, 0.0f, 0.0f);
	info.isDynamic = isDynamic;
	info.isCallback = false;
	info.isPlayer = false;
	info.defunct = false;
	info.shape = shapes[ind];

	if (isDynamic) {
		body->setCcdMotionThreshold(1.e-3f);
		btVector3 aabb1, aabb2;
		body->getAabb(aabb1, aabb2);
		body->setCcdSweptSphereRadius((aabb1.distance(aabb2)) / 10.0f);
		body->setContactProcessingThreshold(0.0f);
	}

	// Find an index for the body
	for (int i = 0; i < bodies.size(); i++) {
		if (bodies[i].defunct) {
			bodies[i] = info;
			body->setUserIndex(i);
			return i;
		}
	}

	bodies.push_back(info);
	body->setUserIndex(bodies.size() - 1);
	return bodies.size() - 1;
}

ConsoleFunction(btSetActive, void, 3, 3, "btSetActive( body, active )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	if (toBool(argv[2]))
		body->forceActivationState(DISABLE_DEACTIVATION);
	else
		body->forceActivationState(DISABLE_SIMULATION);
}

ConsoleFunction(btSetRestitution, void, 3, 3, "btSetRestitution( body, restitution )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->setRestitution((float) atof(argv[2]));
}

ConsoleFunction(btSetFriction, void, 3, 3, "btSetFriction( body, friction )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->setFriction((float)atof(argv[2]));
}

ConsoleFunction(btSetLinearDamping, void, 3, 3, "btSetLinearDamping( body, damping )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->setDamping((float)atof(argv[2]), body->getAngularDamping());
}

ConsoleFunction(btSetAngularDamping, void, 3, 3, "btSetAngularDamping( body, damping )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->setDamping(body->getLinearDamping(), (float)atof(argv[2]));
}

ConsoleFunction(btGetTransform, const char*, 2, 2, "btGetTransform( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return "";
	}
	btRigidBody* body = bodies[ind].body;

	btTransform trans = body->getWorldTransform();
	
	return StringMath::print(btCast<MatrixF>(trans));
}

ConsoleFunction(btSetTransform, void, 3, 3, "btSetTransform( body, mat )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	btTransform trans = btCast<btTransform>(StringMath::scan<MatrixF>(argv[2]));

	if (bodies[ind].isDynamic)
		body->setWorldTransform(trans);
	else
		body->getMotionState()->setWorldTransform(trans);
}

ConsoleFunction(btGetCollisionNormal, const char*, 2, 2, "btGetCollisionNormal( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return "";
	}
	
	btVector3 normal = bodies[ind].collisionNormal;
	return StringMath::print(btCast<Point3F>(normal));
}

ConsoleFunction(btGetCompression, F32, 2, 2, "btGetCompression( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return 0.f;
	}

	return bodies[ind].compression;
}

ConsoleFunction(btGetVelocity, const char*, 2, 2, "btGetVelocity( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return "";
	}
	btRigidBody* body = bodies[ind].body;

	btVector3 vel = btVector3(0.f, 0.f, 0.f);
	
	if (bodies[ind].isDynamic)
		vel = body->getLinearVelocity();
	else {
		for (U32 i = 0; i < moves.size(); i++) {
			if (moves[i]->getBody() == body) {
				vel = moves[i]->getVelocity();
			}
		}
	}

	return StringMath::print(btCast<Point3F>(vel));
}

ConsoleFunction(btSetVelocity, void, 3, 3, "btSetVelocity( body, vel )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	btVector3 vel = btCast<btVector3>(StringMath::scan<Point3F>(argv[2]));

	if (bodies[ind].isDynamic)
		body->setLinearVelocity(vel);
	else {
		for (U32 i = 0; i < moves.size(); i++) {
			if (moves[i]->getBody() == body) {
				moves[i]->setVelocity(vel);
				return;
			}
		}

		TGE::Con::errorf("Movement has not been set for this kinematic body!");
	}
}

ConsoleFunction(btGetAngularVelocity, const char*, 2, 2, "btGetAngularVelocity( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return "";
	}
	btRigidBody* body = bodies[ind].body;

	btVector3 vel = btVector3(0.f, 0.f, 0.f);

	if (bodies[ind].isDynamic)
		vel = body->getAngularVelocity();
	else {
		for (U32 i = 0; i < moves.size(); i++) {
			if (moves[i]->getBody() == body) {
				vel = moves[i]->getAngularVelocity();
			}
		}
	}

	return StringMath::print(btCast<Point3F>(vel));
}

ConsoleFunction(btSetAngularVelocity, void, 3, 3, "btSetAngularVelocity( body, vel )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	btVector3 vel = btCast<btVector3>(StringMath::scan<Point3F>(argv[2]));

	if (bodies[ind].isDynamic)
		body->setAngularVelocity(vel);
	else {
		for (U32 i = 0; i < moves.size(); i++) {
			if (moves[i]->getBody() == body) {
				moves[i]->setAngularVelocity(vel);
				return;
			}
		}

		TGE::Con::errorf("Movement has not been set for this kinematic body!");
	}
}

ConsoleFunction(btSetFactor, void, 3, 3, "btSetFactor( body, fac )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	btVector3 fac = btCast<btVector3>(StringMath::scan<Point3F>(argv[2]));

	body->setLinearFactor(fac);
}

ConsoleFunction(btApplyCentralImpulse, void, 3, 3, "btApplyCentralImpulse( body, vec )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->applyCentralImpulse(btCast<btVector3>(StringMath::scan<Point3F>(argv[2])));
}

ConsoleFunction(btApplyTorqueImpulse, void, 3, 3, "btApplyTorqueImpulse( body, vec )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->applyTorqueImpulse(btCast<btVector3>(StringMath::scan<Point3F>(argv[2])));
}

ConsoleFunction(btApplyCentralForce, void, 3, 3, "btApplyCentralForce( body, vec )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->applyCentralForce(btCast<btVector3>(StringMath::scan<Point3F>(argv[2])));
}

ConsoleFunction(btApplyTorque, void, 3, 3, "btApplyTorque( body, vec )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	body->applyTorque(btCast<btVector3>(StringMath::scan<Point3F>(argv[2])));
}

ConsoleFunction(btRaycast, const char*, 3, 4, "btRaycast( start, end, [ignore] )") {
	btVector3 start = btCast<btVector3>(StringMath::scan<Point3F>(argv[1]));
	btVector3 end = btCast<btVector3>(StringMath::scan<Point3F>(argv[2]));
	int ind = -1;
	if (argc > 3) {
		ind = atoi(argv[3]);
		if (ind < 0 || ind >= (int)bodies.size()) {
			TGE::Con::errorf("Invalid body handle!");
			char *str = TGE::Con::getReturnBuffer(1);
			strcpy_s(str, 1, "");
			return str;
		}
	}
	if (start.distance(end) < 0.001f) {
		TGE::Con::errorf("Points are too close!");
		char *str = TGE::Con::getReturnBuffer(1);
		strcpy_s(str, 1, "");
		return str;
	}
	btCollisionWorld::AllHitsRayResultCallback callback(start, end);
	dynamicsWorld->rayTest(start, end, callback);
	if (!callback.hasHit()) {
		char *str = TGE::Con::getReturnBuffer(1);
		strcpy_s(str, 1, "");
		return str;
	}

	for (int i = 0; i < callback.m_collisionObjects.size(); i++) {
		const btCollisionObject *body = callback.m_collisionObjects[i];
		if (ind >= 0 && body == bodies[ind].body)
			continue;

		return StringMath::print2<Point3F>(btCast<Point3F>(callback.m_hitPointWorld[i]), btCast<Point3F>(callback.m_hitNormalWorld[i]));
	}

	char *str = TGE::Con::getReturnBuffer(1);
	strcpy_s(str, 1, "");
	return str;
}

TorqueOverrideMember(void, Marble::advancePhysics, (TGE::Marble *thisObj, const TGE::Move *move, U32 delta), originalAdvancePhysics)
{
	if (!TGE::Con::getBoolVariable("$Core::PhysicsDisabled"))
		originalAdvancePhysics(thisObj, move, delta);
	else
		TGE::Members::Marble::advanceCamera(thisObj, move, delta);
}

TorqueOverrideMember(void, Marble::setTransform, (TGE::Marble *thisObj, const MatrixF &mat), originalSetTransform)
{
	TGE::Members::SceneObject::setTransform(thisObj, mat);
}

// -----------------------------------------------------------------------------------

ConsoleFunction(btCreateLinearMovement, void, 2, 2, "btCreateLinearMovement( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	moves.push_back(new VelocityMovement(body));
}

ConsoleFunction(btCreateParentMovement, void, 3, 4, "btCreateParentMovement( body, parent, [offset] )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;
	U32 ind2 = atoi(argv[2]);
	if (ind2 < 0 || ind2 >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body2 = bodies[ind2].body;

	btTransform offset;

	if (argc < 4) {
		offset = body->getWorldTransform();
		btTransform transParent; // lol puns
		transParent = body2->getWorldTransform();
		offset = transParent.inverse() * offset;
	}
	else {
		offset = btCast<btTransform>(StringMath::scan<MatrixF>(argv[3]));
	}

	moves.push_back(new ParentMovement(body, body2, offset));
}

ConsoleFunction(btCreatePathedMovement, void, 3, 4, "btCreatePathedMovement( body, path, [offset] )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;
	TGE::SimGroup *path = static_cast<TGE::SimGroup*>(TGE::Sim::findObject(argv[2]));

	if (path == NULL) {
		TGE::Con::errorf("Path not found!");
		return;
	}

	btTransform offset;

	if (argc < 4) {
		offset = btTransform::getIdentity();
	}
	else {
		offset = btCast<btTransform>(StringMath::scan<MatrixF>(argv[3]));
	}

	moves.push_back(new PathedMovement(body, path, offset));
}

ConsoleFunction(btSetTargetPosition, void, 3, 3, "btSetTargetPosition( body, ms )") {
	U32 ind = atoi(argv[1]);
	U32 targetTime = atoi(argv[2]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	for (U32 i = 0; i < moves.size(); i++) {
		if (moves[i]->getBody() == body) {
			PathedMovement *move = dynamic_cast<PathedMovement*>(moves[i]);
			if (move == NULL)
				continue;

			move->setTargetTimeMS(targetTime);
			return;
		}
	}

	TGE::Con::errorf("Body does not have a path defined!");
	return;
}

ConsoleFunction(btGetPathPosition, F32, 2, 2, "btGetPathPosition( body )") {
	U32 ind = atoi(argv[1]);
	U32 targetTime = atoi(argv[2]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return 0.0f;
	}
	btRigidBody* body = bodies[ind].body;

	for (U32 i = 0; i < moves.size(); i++) {
		if (moves[i]->getBody() == body) {
			PathedMovement *move = dynamic_cast<PathedMovement*>(moves[i]);
			if (move == NULL)
				continue;

			return move->getPathTimeMS();
		}
	}

	TGE::Con::errorf("Body does not have a path defined!");
	return 0.0f;
}

ConsoleFunction(btCreateApproachMovement, void, 4, 4, "btCreateApproachMovement( body, speed, rotSpeed )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	moves.push_back(new ApproachMovement(body, (float)atof(argv[2]), (float)atof(argv[3])));
}

ConsoleFunction(btSetTargetTransform, void, 3, 3, "btSetTargetTransform( body, matrix )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;

	btTransform trans = btCast<btTransform>(StringMath::scan<MatrixF>(argv[2]));

	for (U32 i = 0; i < moves.size(); i++) {
		if (moves[i]->getBody() == body) {
			ApproachMovement *move = dynamic_cast<ApproachMovement*>(moves[i]);
			if (move == NULL)
				continue;

			move->setTargetTransform(trans);
			return;
		}
	}

	TGE::Con::errorf("Body does not have an approach defined!");
	return;
}

ConsoleFunction(btDeleteMovement, void, 2, 2, "btDeleteMovement( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody* body = bodies[ind].body;
	
	for (U32 i = 0; i < moves.size(); i++) {
		if (moves[i]->getBody() == body) {
			moves.erase(moves.begin() + i);
			return;
		}
	}
}

ConsoleFunction(btEnableCallback, void, 2, 2, "btEnableCallback( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	bodies[ind].isCallback = true;
}

ConsoleFunction(btEnablePlayer, void, 2, 2, "btEnablePlayer( body )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	bodies[ind].isPlayer = true;
}

ConsoleFunction(btSetSolid, void, 3, 3, "btSetSolid( body, solid )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= bodies.size()) {
		TGE::Con::errorf("Invalid body handle!");
		return;
	}
	btRigidBody *body = bodies[ind].body;

	if (toBool(argv[2]))
		body->setCollisionFlags(body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
	else
		body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
}

std::string getBodyInfo(int i) {
	std::ostringstream s;
	s << "Body #";
	s << i;
	s << ": ";

	if (i < 0 || i >= bodies.size()) {
		s << "<index out of bounds>";
	}
	else if (bodies[i].defunct) {
		s << "<defunct>";
	}
	else {
		const BodyInfo &b = bodies[i];
		// Print body info
		if (!b.isDynamic)
			s << "static ";
		else
			s << "dynamic ";
		if (b.isPlayer)
			s << "player ";
		if (b.isCallback)
			s << "onCollision-able ";

		const ShapeInfo &sh = b.shape;

		switch (sh.shape->getShapeType()) {
		case BOX_SHAPE_PROXYTYPE:
			s << "box";
			break;
		case CYLINDER_SHAPE_PROXYTYPE:
			s << "cylinder";
			break;
		case EMPTY_SHAPE_PROXYTYPE:
			s << "empty";
			break;
		case SPHERE_SHAPE_PROXYTYPE:
			s << "sphere";
			break;
		case TRIANGLE_MESH_SHAPE_PROXYTYPE:
			s << "triangle mesh";
			break;
		default:
			s << "unknown shape";
		}

		s << " mass = ";
		s << 1.0f / b.body->getInvMass(); // *really,* Bullet? *really?*
		s << " transform = (";
		btTransform t = b.body->getWorldTransform();
		btVector3 tp = t.getOrigin();
		s << tp.getX() << ", " << tp.getY() << ", " << tp.getZ() << ", ";
		btQuaternion tq = t.getRotation();
		s << tq.getAxis().getX() << ", "
			<< tq.getAxis().getY() << ", "
			<< tq.getAxis().getZ() << ", "
			<< tq.getAngle() << ")";
	}

	return s.str();
}

ConsoleFunction(btDumpBody, void, 2, 2, "btDumpBody( body )") {
	U32 ind = atoi(argv[1]);
	TGE::Con::printf("%s", getBodyInfo(ind).c_str());
}

ConsoleFunction(btDumpAllBodies, void, 1, 1, "btDumpAllBodies( )") {
	TGE::Con::printf("ALL BODIES\n----------\n");
	for (int ind = 0; ind < bodies.size(); ind++)
		TGE::Con::printf("%s", getBodyInfo(ind).c_str());
	TGE::Con::printf("");
}