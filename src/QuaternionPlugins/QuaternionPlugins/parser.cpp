#include "stdafx.h"
#include <sstream>
#include <iostream>

extern "C" {
	typedef struct yy_buffer_state *YY_BUFFER_STATE;
	int yyparse();
	YY_BUFFER_STATE yy_scan_string(char *yy_str);
	void yy_delete_buffer(YY_BUFFER_STATE b);
}

extern "C" void conprintf(const char *format, ...);
extern "C" void conwarnf(const char *format, ...);
extern "C" void conerrorf(const char *format, ...);

extern "C" void setparsedglobal(const char *str);

void conprintf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	TGE::Con::_printf(TGE::ConsoleLogEntry::Normal, TGE::ConsoleLogEntry::General, format, args);
	va_end(args);
}

void conwarnf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	TGE::Con::_printf(TGE::ConsoleLogEntry::Warning, TGE::ConsoleLogEntry::General, format, args);
	va_end(args);
}

void conerrorf(const char *format, ...) {
	va_list args;
	va_start(args, format);
	TGE::Con::_printf(TGE::ConsoleLogEntry::Error, TGE::ConsoleLogEntry::General, format, args);
	va_end(args);
}

std::string globalParsed = std::string();

void setparsedglobal(const char *str) {
	globalParsed.assign(str);
}

void parse(const char *input) {
	char *buffer = (char*)malloc(strlen(input) + 7);
	strcpy(buffer, "}# ");
	strcat(buffer, input);
	strcat(buffer, " #{");
	YY_BUFFER_STATE my_string_buffer = yy_scan_string(buffer);
	int my_parse_result = yyparse();
	yy_delete_buffer(my_string_buffer);
	free(buffer);
}

ConsoleFunction(preprocess, const char*, 2, 2, "preprocess( script )") {
	parse(argv[1]);
	char *ret = TGE::Con::getReturnBuffer(globalParsed.size() + 1);
	strcpy_s(ret, globalParsed.size() + 1, globalParsed.c_str());
	return ret;
}

TorqueOverride(const char*, ceval, (TGE::SimObject *object, int argc, const char **argv), originalceval) {
	parse(argv[1]);
	argv[1] = globalParsed.c_str();
	return originalceval(object, argc, argv);
}