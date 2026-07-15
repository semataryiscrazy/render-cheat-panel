#pragma once

int ka_init();
int ka_login(const char* username, const char* password);
int ka_register(const char* username, const char* password, const char* key);
int ka_verify();
const char* ka_get_username();
const char* ka_get_error();
int ka_get_days_remaining();
const char* ka_get_token();
void ka_set_api_url(const char* url);
void ka_cleanup();
