

#include "tiffiop.h"

TIFFErrorHandlerExt _TIFFwarningHandlerExt = NULL;

TIFFErrorHandler
TIFFSetWarningHandler(TIFFErrorHandler handler)
{
	TIFFErrorHandler prev = _TIFFwarningHandler;
	_TIFFwarningHandler = handler;
	return (prev);
}

TIFFErrorHandlerExt
TIFFSetWarningHandlerExt(TIFFErrorHandlerExt handler)
{
	TIFFErrorHandlerExt prev = _TIFFwarningHandlerExt;
	_TIFFwarningHandlerExt = handler;
	return (prev);
}

void
TIFFWarning(const char* module, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (_TIFFwarningHandler)
		(*_TIFFwarningHandler)(module, fmt, ap);
	if (_TIFFwarningHandlerExt)
		(*_TIFFwarningHandlerExt)(0, module, fmt, ap);
	va_end(ap);
}

void
TIFFWarningExt(thandle_t fd, const char* module, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (_TIFFwarningHandler)
		(*_TIFFwarningHandler)(module, fmt, ap);
	if (_TIFFwarningHandlerExt)
		(*_TIFFwarningHandlerExt)(fd, module, fmt, ap);
	va_end(ap);
}

