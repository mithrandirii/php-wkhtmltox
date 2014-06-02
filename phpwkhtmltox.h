#ifndef PHP_PHPWKHTMLTOX_H
#define PHP_PHPWKHTMLTOX_H

#define PHP_PHPWKHTMLTOX_VERSION "0.10"
#define PHP_PHPWKHTMLTOX_EXTNAME "phpwkhtmltox"

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(phpwkhtmltox)
int wkhtmltoimage_initialized;
int wkhtmltopdf_initialized;
ZEND_END_MODULE_GLOBALS(phpwkhtmltox)

#ifdef ZTS
#define PHPWKHTMLTOX_G(v) TSRMG(phpwkhtmltox_globals_id, zend_phpwkhtmltox_globals *, v)
#else
#define PHPWKHTMLTOX_G(v) (phpwkhtmltox_globals.v)
#endif

PHP_FUNCTION(wkhtmltox_convert);

struct wkhtmltopdf_converter;
typedef struct wkhtmltopdf_converter wkhtmltopdf_converter;
typedef int (*fp)(void *settings, const char *name, const char *value);
typedef void(*wkhtmltopdf_str_callback)(wkhtmltopdf_converter *c, const char *msg);
int wkhtmltox_set_params(void *settings, fp set_function, zval *params, zval *input);
void wkhtmltox_warning(wkhtmltopdf_converter *c, const char *msg);
void wkhtmltox_error(wkhtmltopdf_converter *c, const char *msg);

extern zend_module_entry phpwkhtmltox_module_entry;
#define phpext_phpwkhtmltox_ptr &phpwkhtmltox_module_entry

#endif
