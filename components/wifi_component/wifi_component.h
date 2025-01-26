#include <stdint.h>
#include <esp_err.h>

// Functions to get security parameters
esp_err_t example_get_sec2_salt(const char **salt, uint16_t *salt_len);
esp_err_t example_get_sec2_verifier(const char **verifier, uint16_t *verifier_len);
// Functions to handle ThingsBoard URL storage in NVS
esp_err_t save_thingsboard_url(const char *url);
esp_err_t load_thingsboard_url(char *url, size_t max_len);

// Function to handle provisioning events
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// Function to handle ThingsBoard URL received during provisioning
esp_err_t thingsboard_url_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, 
                                  uint8_t **outbuf, ssize_t *outlen, void *priv_data);

// Function to generate the device service name
void get_device_service_name(char *service_name, size_t max);
// Initialize nvs flash memory
esp_err_t nvs_initialize(void);
// Main provisioning and connection function
void provision_and_connect(void);
extern char thingsboard_url[12];
