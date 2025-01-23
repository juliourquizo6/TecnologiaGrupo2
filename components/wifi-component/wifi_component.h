#include <stdint.h>
#include <esp_err.h>

void provision_and_connect(void);
esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                   uint8_t **outbuf, ssize_t *outlen, void *priv_data);
void get_device_service_name(char *service_name, size_t max);
esp_err_t example_get_sec2_salt(const uint8_t **salt, size_t *salt_len);
esp_err_t example_get_sec2_verifier(const uint8_t **verifier, size_t *verifier_len);