/*
** Impl details for the JSON API which need to be shared
** across multiple C files.
*/

/*
** The "official" list of Fossil/JSON error codes.
** Their values might very well change during initial
** development but after their first public release
** they must stay stable.
** Values must be in the range 1..9999
**
** Numbers evenly dividable by 100 are "categories",
** and error codes for a given category have their
** high bits set to the category value.
**
*/
enum FossilJsonCodes {

FSL_JSON_E_GENERIC = 1000,
FSL_JSON_E_GENERIC_SUB1 = FSL_JSON_E_GENERIC + 100,
FSL_JSON_E_INVALID_REQUEST = FSL_JSON_E_GENERIC_SUB1 + 1,
FSL_JSON_E_UNKNOWN_COMMAND = FSL_JSON_E_GENERIC_SUB1 + 2,
FSL_JSON_E_UNKNOWN = FSL_JSON_E_GENERIC_SUB1 + 3,
FSL_JSON_E_RESOURCE_NOT_FOUND = FSL_JSON_E_GENERIC_SUB1 + 4,
FSL_JSON_E_TIMEOUT = FSL_JSON_E_GENERIC_SUB1 + 5,
FSL_JSON_E_ASSERT = FSL_JSON_E_GENERIC_SUB1 + 6,
FSL_JSON_E_ALLOC = FSL_JSON_E_GENERIC_SUB1 + 7,
FSL_JSON_E_NYI = FSL_JSON_E_GENERIC_SUB1 + 8,
FSL_JSON_E_PANIC = FSL_JSON_E_GENERIC_SUB1 + 9,

FSL_JSON_E_AUTH = 2000,
FSL_JSON_E_MISSING_AUTH = FSL_JSON_E_AUTH + 1,
FSL_JSON_E_DENIED = FSL_JSON_E_AUTH + 2,
FSL_JSON_E_WRONG_MODE = FSL_JSON_E_AUTH + 3,

FSL_JSON_E_LOGIN_FAILED = FSL_JSON_E_AUTH + 100,
FSL_JSON_E_LOGIN_FAILED_NONAME = FSL_JSON_E_LOGIN_FAILED + 1,
FSL_JSON_E_LOGIN_FAILED_NOPW = FSL_JSON_E_LOGIN_FAILED + 2,
FSL_JSON_E_LOGIN_FAILED_NOTFOUND = FSL_JSON_E_LOGIN_FAILED + 3,

FSL_JSON_E_USAGE = 3000,
FSL_JSON_E_INVALID_ARGS = FSL_JSON_E_USAGE + 1,
FSL_JSON_E_MISSING_ARGS = FSL_JSON_E_USAGE + 2,

FSL_JSON_E_DB = 4000,
FSL_JSON_E_STMT_PREP = FSL_JSON_E_DB + 1,
FSL_JSON_E_STMT_BIND = FSL_JSON_E_DB + 2,
FSL_JSON_E_STMT_EXEC = FSL_JSON_E_DB + 3,
FSL_JSON_E_DB_LOCKED = FSL_JSON_E_DB + 4,

FSL_JSON_E_DB_NEEDS_REBUILD = FSL_JSON_E_DB + 101
};

