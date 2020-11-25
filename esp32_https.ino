#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFiSettings.h>

#include "esp_http_client.h"

#include "editline.h"
#include "cmdproc.h"

#define printf Serial.printf

static char line[256];

static void show_help(const cmd_t * cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        printf("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_help(int argc, char *argv[]);

static int do_https_post(int argc, char *argv[])
{
    static char rxbuffer[2000];
    const char *url = (argc > 1) ? argv[1] : "https://ptsv2.com/t/bertrik/post";
    int timeout = 3000;

    esp_http_client_config_t config = { };
    config.url = url;
    config.timeout_ms = timeout;
    esp_http_client_handle_t client = esp_http_client_init(&config);

    const char *post_data = "Awesome!";
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        printf("HTTPS Status = %d, content_length = %d\n",
               esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
    } else {
        printf("Error perform http request %s\n", esp_err_to_name(err));
    }

    int bufsize = sizeof(rxbuffer);
    int totalsize = 0;
    char *p = rxbuffer;
    int len = 0;
    do {
        len = esp_http_client_read(client, p, bufsize);
        if (len < 0) {
            break;
        }
        p += len;
        totalsize += len;
        bufsize -= len;
    }
    while (len > 0);
    esp_http_client_cleanup(client);

    // show contents of rxbuffer
    for (int i = 0; i < totalsize; i++) {
        char c = rxbuffer[i];
        printf("%c", isprint(c) ? c : '.');
    }
    printf("\n");

    return (err == ESP_OK) ? CMD_OK : 1;
}

static const cmd_t commands[] = {
    { "help", do_help, "Show help" },
    { "post", do_https_post, "[url] Send a HTTPS POST to the specified URL" },
    { NULL, NULL, NULL }
};

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return CMD_OK;
}

void setup(void)
{
    Serial.begin(115200);
    printf("\nESP-HTTPS\n");
    EditInit(line, sizeof(line));

    SPIFFS.begin(true);
    WiFiSettings.connect();
}

void loop(void)
{
    // parse command line
    bool haveLine = false;
    if (Serial.available()) {
        char c;
        haveLine = EditLine(Serial.read(), &c);
        Serial.write(c);
    }
    if (haveLine) {
        int result = cmd_process(commands, line);
        switch (result) {
        case CMD_OK:
            printf("OK\n");
            break;
        case CMD_NO_CMD:
            break;
        case CMD_UNKNOWN:
            printf("Unknown command, available commands:\n");
            show_help(commands);
            break;
        default:
            printf("%d\n", result);
            break;
        }
        printf(">");
    }
}
