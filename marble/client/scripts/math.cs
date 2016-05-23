// position from a transform
function MatrixPos(%mat) {
   return getWords(%mat,0,2);
}

// rotation from a transform
function MatrixRot(%mat) {
   return getWords(%mat,3);
}

// right vector from a transform
function MatrixRight(%mat) {
   return MatrixMulVector(%mat,"1 0 0");
}

// forward vector from a transform
function MatrixForward(%mat) {
   return MatrixMulVector(%mat,"0 1 0");
}

// up vector from a transform
function MatrixUp(%mat) {
   return MatrixMulVector(%mat,"0 0 1");
}