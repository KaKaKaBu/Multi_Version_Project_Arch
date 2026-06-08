/**
 * @file cJSON_config.h
 * @brief 内嵌 cJSON 的功能裁剪开关（Flash 优化）。
 *
 * CJSON_EMBEDDED_MINIMAL=1（默认）时禁用数组、Duplicate、Compare 等 API；
 * 设为 0 恢复完整库。CJSON_NESTING_LIMIT 限制解析深度以防栈溢出。
 * 业务代码请优先使用 cjson_port.h，勿直接依赖已禁用的符号。
 */

#ifndef CJSON_CONFIG_H
#define CJSON_CONFIG_H

/** @brief 为 1 时禁用可选 cJSON API，以减小 Flash 占用。 */
#ifndef CJSON_EMBEDDED_MINIMAL
#define CJSON_EMBEDDED_MINIMAL 1
#endif

/** @brief 解析器接受的最大 JSON 嵌套深度。 */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 32
#endif

#if CJSON_EMBEDDED_MINIMAL

/** @brief 为 1 时禁用 cJSON_Version()。 */
#ifndef CJSON_DISABLE_VERSION
#define CJSON_DISABLE_VERSION 1
#endif
/** @brief 为 1 时禁用 cJSON_Get* 辅助函数。 */
#ifndef CJSON_DISABLE_GET_HELPERS
#define CJSON_DISABLE_GET_HELPERS 1
#endif
/** @brief 为 1 时禁用 cJSON_Set* 辅助函数。 */
#ifndef CJSON_DISABLE_SET_HELPERS
#define CJSON_DISABLE_SET_HELPERS 1
#endif
/** @brief 为 1 时禁用扩展解析 API。 */
#ifndef CJSON_DISABLE_PARSE_API
#define CJSON_DISABLE_PARSE_API 1
#endif
/** @brief 为 1 时禁用 cJSON_Print 格式化输出。 */
#ifndef CJSON_DISABLE_FORMATTED_PRINT
#define CJSON_DISABLE_FORMATTED_PRINT 1
#endif
/** @brief 为 1 时禁用缓冲式打印辅助函数。 */
#ifndef CJSON_DISABLE_PRINT_BUFFERED
#define CJSON_DISABLE_PRINT_BUFFERED 1
#endif
/** @brief 为 1 时禁用 JSON 数组节点支持。 */
#ifndef CJSON_DISABLE_ARRAYS
#define CJSON_DISABLE_ARRAYS 1
#endif
/** @brief 为 1 时禁用 raw JSON 节点支持。 */
#ifndef CJSON_DISABLE_RAW
#define CJSON_DISABLE_RAW 1
#endif
/** @brief 为 1 时禁用 bool/null 打印变体。 */
#ifndef CJSON_DISABLE_BOOL_NULL_PRINT
#define CJSON_DISABLE_BOOL_NULL_PRINT 1
#endif
/** @brief 为 1 时禁用数组操作 API。 */
#ifndef CJSON_DISABLE_ARRAY_API
#define CJSON_DISABLE_ARRAY_API 1
#endif
/** @brief 为 1 时禁用 cJSON 工具辅助函数。 */
#ifndef CJSON_DISABLE_UTILS
#define CJSON_DISABLE_UTILS 1
#endif
/** @brief 为 1 时禁用额外的 object/array 创建辅助函数。 */
#ifndef CJSON_DISABLE_CREATE_EXTRAS
#define CJSON_DISABLE_CREATE_EXTRAS 1
#endif
/** @brief 为 1 时禁用 JSON 引用节点。 */
#ifndef CJSON_DISABLE_REFERENCE
#define CJSON_DISABLE_REFERENCE 1
#endif
/** @brief 为 1 时禁用 cJSON_Duplicate。 */
#ifndef CJSON_DISABLE_DUPLICATE
#define CJSON_DISABLE_DUPLICATE 1
#endif
/** @brief 为 1 时禁用 cJSON_Minify。 */
#ifndef CJSON_DISABLE_MINIFY
#define CJSON_DISABLE_MINIFY 1
#endif
/** @brief 为 1 时禁用 cJSON_Compare。 */
#ifndef CJSON_DISABLE_COMPARE
#define CJSON_DISABLE_COMPARE 1
#endif
/** @brief 为 1 时禁用 cJSON_Is* 类型测试辅助函数。 */
#ifndef CJSON_DISABLE_IS_HELPERS
#define CJSON_DISABLE_IS_HELPERS 1
#endif

#else /* !CJSON_EMBEDDED_MINIMAL */

/** @brief 为 1 时禁用 cJSON_Version()。 */
#ifndef CJSON_DISABLE_VERSION
#define CJSON_DISABLE_VERSION 0
#endif
/** @brief 为 1 时禁用 cJSON_Get* 辅助函数。 */
#ifndef CJSON_DISABLE_GET_HELPERS
#define CJSON_DISABLE_GET_HELPERS 0
#endif
/** @brief 为 1 时禁用 cJSON_Set* 辅助函数。 */
#ifndef CJSON_DISABLE_SET_HELPERS
#define CJSON_DISABLE_SET_HELPERS 0
#endif
/** @brief 为 1 时禁用扩展解析 API。 */
#ifndef CJSON_DISABLE_PARSE_API
#define CJSON_DISABLE_PARSE_API 0
#endif
/** @brief 为 1 时禁用 cJSON_Print 格式化输出。 */
#ifndef CJSON_DISABLE_FORMATTED_PRINT
#define CJSON_DISABLE_FORMATTED_PRINT 0
#endif
/** @brief 为 1 时禁用缓冲式打印辅助函数。 */
#ifndef CJSON_DISABLE_PRINT_BUFFERED
#define CJSON_DISABLE_PRINT_BUFFERED 0
#endif
/** @brief 为 1 时禁用 JSON 数组节点支持。 */
#ifndef CJSON_DISABLE_ARRAYS
#define CJSON_DISABLE_ARRAYS 0
#endif
/** @brief 为 1 时禁用 raw JSON 节点支持。 */
#ifndef CJSON_DISABLE_RAW
#define CJSON_DISABLE_RAW 0
#endif
/** @brief 为 1 时禁用 bool/null 打印变体。 */
#ifndef CJSON_DISABLE_BOOL_NULL_PRINT
#define CJSON_DISABLE_BOOL_NULL_PRINT 0
#endif
/** @brief 为 1 时禁用数组操作 API。 */
#ifndef CJSON_DISABLE_ARRAY_API
#define CJSON_DISABLE_ARRAY_API 0
#endif
/** @brief 为 1 时禁用 cJSON 工具辅助函数。 */
#ifndef CJSON_DISABLE_UTILS
#define CJSON_DISABLE_UTILS 0
#endif
/** @brief 为 1 时禁用额外的 object/array 创建辅助函数。 */
#ifndef CJSON_DISABLE_CREATE_EXTRAS
#define CJSON_DISABLE_CREATE_EXTRAS 0
#endif
/** @brief 为 1 时禁用 JSON 引用节点。 */
#ifndef CJSON_DISABLE_REFERENCE
#define CJSON_DISABLE_REFERENCE 0
#endif
/** @brief 为 1 时禁用 cJSON_Duplicate。 */
#ifndef CJSON_DISABLE_DUPLICATE
#define CJSON_DISABLE_DUPLICATE 0
#endif
/** @brief 为 1 时禁用 cJSON_Minify。 */
#ifndef CJSON_DISABLE_MINIFY
#define CJSON_DISABLE_MINIFY 0
#endif
/** @brief 为 1 时禁用 cJSON_Compare。 */
#ifndef CJSON_DISABLE_COMPARE
#define CJSON_DISABLE_COMPARE 0
#endif
/** @brief 为 1 时禁用 cJSON_Is* 类型测试辅助函数。 */
#ifndef CJSON_DISABLE_IS_HELPERS
#define CJSON_DISABLE_IS_HELPERS 0
#endif

#endif /* CJSON_EMBEDDED_MINIMAL */

#endif /* CJSON_CONFIG_H */
