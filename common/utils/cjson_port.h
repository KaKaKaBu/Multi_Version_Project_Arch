/**
 * @file cjson_port.h
 * @brief cJSON 工程封装：钩子、解析/释放、遥测 JSON 构建。
 *
 * 使用前调用 cjson_port_init() 将 cJSON 绑定到 mem_pool 提供的 malloc/free。
 * 解析与打印分配的内存须分别用 cjson_release / cjson_release_string 释放。
 */

#ifndef CJSON_PORT_H
#define CJSON_PORT_H

#include "cJSON.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 安装 malloc/free 钩子，使 cJSON 使用项目堆（mem_pool）。 */
void cjson_port_init(void);

/**
 * @brief 解析 JSON 文本（支持内嵌 \\0 时由 length 限定，避免依赖 strlen）。
 * @return 树根节点；失败返回 null，调用方无需 release。
 */
cJSON *cjson_parse(const char *text, size_t length);

/** @brief 释放 cjson_parse 或 cJSON_Create* 得到的树。 */
void cjson_release(cJSON *item);

/** @brief 释放 cJSON_Print* / cjson_build_telemetry 返回的堆字符串。 */
void cjson_release_string(char *text);

/**
 * @brief 构建紧凑遥测 JSON：{"device":"...","temperature":x,"humidity":y}。
 * @return 堆字符串，用后 cjson_release_string；失败返回 null。
 */
char *cjson_build_telemetry(const char *device, float temperature, float humidity);

#ifdef __cplusplus
}
#endif

#endif
