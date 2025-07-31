esp_err_t example_get_sec2_salt(const char **salt, uint16_t *salt_len);
esp_err_t example_get_sec2_verifier(const char **verifier, uint16_t *verifier_len);

wifi_prov_security2_params_t get_prov_security2_params();
const char* get_username();
const char* get_pwd();
