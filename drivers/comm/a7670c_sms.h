/**
 * @file a7670c_sms.h
 * @brief A7670C 4G 模块短信报警 helper API。
 */

#ifndef A7670C_SMS_H
#define A7670C_SMS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 初始化 A7670C AT 短信模式。 */
int a7670c_sms_setup(void);

/** @brief 返回短信模式是否已经初始化成功。 */
int a7670c_sms_is_ready(void);

/**
 * @brief 发送文本短信。
 * @param phone 目标手机号，ASCII 数字/加号字符串。
 * @param text 短信正文，建议使用 ASCII 文本字段。
 * @return 0 成功，负值失败。
 */
int a7670c_sms_send_text(const char *phone, const char *text);

/** @brief 轮询 UART RX，将新数据放入 AT 响应缓冲。 */
void a7670c_sms_poll(void);

#ifdef __cplusplus
}
#endif

#endif
