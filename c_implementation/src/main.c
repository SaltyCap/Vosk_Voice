#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <libwebsockets.h>
#include "vosk_server.h"

static volatile int interrupted = 0;
static struct lws_context *context = NULL;

// Signal handler for graceful shutdown
static void sigint_handler(int sig) {
    interrupted = 1;
    if (context) {
        lws_cancel_service(context);
    }
}

// Per-session data
struct per_session_data {
    session_data_t *vosk_session;
};

// WebSocket callback for /audio endpoint
static int callback_audio(struct lws *wsi, enum lws_callback_reasons reason,
                          void *user, void *in, size_t len) {
    struct per_session_data *pss = (struct per_session_data *)user;

    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("Client connected\n");
            pss->vosk_session = create_session();
            if (!pss->vosk_session) {
                fprintf(stderr, "Failed to create session\n");
                return -1;
            }
            break;

        case LWS_CALLBACK_RECEIVE:
            if (!pss->vosk_session) {
                fprintf(stderr, "No session data\n");
                return -1;
            }

            // Check if it's a text message (control command)
            if (lws_frame_is_binary(wsi) == 0) {
                char *msg = (char *)in;

                if (strncmp(msg, "start", 5) == 0) {
                    printf("\n--- Recording Started ---\n");
                    start_recording(pss->vosk_session);
                }
                else if (strncmp(msg, "stop", 4) == 0) {
                    printf("\n--- Recording Stopped ---\n");
                    char *final_result = NULL;
                    stop_recording(pss->vosk_session, &final_result);

                    if (final_result) {
                        // Send final result to client
                        size_t final_len = strlen(final_result);
                        unsigned char *buf = malloc(LWS_PRE + final_len);
                        if (buf) {
                            memcpy(buf + LWS_PRE, final_result, final_len);
                            lws_write(wsi, buf + LWS_PRE, final_len, LWS_WRITE_TEXT);
                            free(buf);
                        }
                        free(final_result);
                    }
                }
            }
            // Binary data (audio)
            else if (pss->vosk_session->recording) {
                char *result_json = NULL;
                bool is_final = false;

                int ret = handle_audio_data(pss->vosk_session, (const uint8_t *)in, len,
                                           &result_json, &is_final);

                if (ret == 0 && result_json) {
                    // Send result to client
                    size_t json_len = strlen(result_json);
                    unsigned char *buf = malloc(LWS_PRE + json_len);
                    if (buf) {
                        memcpy(buf + LWS_PRE, result_json, json_len);
                        lws_write(wsi, buf + LWS_PRE, json_len, LWS_WRITE_TEXT);
                        free(buf);
                    }
                    free(result_json);
                }
            }
            break;

        case LWS_CALLBACK_CLOSED:
            printf("Client disconnected\n");
            if (pss->vosk_session) {
                destroy_session(pss->vosk_session);
                pss->vosk_session = NULL;
            }
            break;

        default:
            break;
    }

    return 0;
}

// HTTP callback for serving static files
static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_HTTP:
            // Serve index.html for root path
            if (strcmp((const char *)in, "/") == 0) {
                if (lws_serve_http_file(wsi, "static/index.html", "text/html", NULL, 0))
                    return -1;
            }
            break;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            return -1; // Close connection after file is sent

        default:
            break;
    }

    return 0;
}

// Protocol definitions
static struct lws_protocols protocols[] = {
    {
        "http",
        callback_http,
        0,
        0,
        0,
        NULL,
        0
    },
    {
        "audio-protocol",
        callback_audio,
        sizeof(struct per_session_data),
        MAX_PAYLOAD_SIZE,
        0,
        NULL,
        0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 } // Terminator
};

// Mount point for static files
static const struct lws_http_mount mount = {
    .mount_next = NULL,
    .mountpoint = "/",
    .origin = "./static",
    .def = "index.html",
    .protocol = NULL,
    .cgienv = NULL,
    .extra_mimetypes = NULL,
    .interpret = NULL,
    .cgi_timeout = 0,
    .cache_max_age = 0,
    .auth_mask = 0,
    .cache_reusable = 0,
    .cache_revalidate = 0,
    .cache_intermediaries = 0,
    .origin_protocol = LWSMPRO_FILE,
    .mountpoint_len = 1,
    .basic_auth_login_file = NULL,
};

int main(int argc, char **argv) {
    struct lws_context_creation_info info;
    int port = DEFAULT_PORT;
    const char *cert_path = "cert.pem";
    const char *key_path = "key.pem";

    printf("===========================================\n");
    printf("  Vosk Voice Transcription Server (C)\n");
    printf("  Optimized for Raspberry Pi 5\n");
    printf("===========================================\n\n");

    // Setup signal handler
    signal(SIGINT, sigint_handler);

    // Initialize Vosk model
    printf("Loading Vosk model from '%s'...\n", MODEL_PATH);
    if (init_vosk_model(MODEL_PATH) != 0) {
        fprintf(stderr, "Failed to load Vosk model\n");
        fprintf(stderr, "Please make sure the 'model' folder exists and contains the Vosk model files.\n");
        return 1;
    }
    printf("Model loaded successfully.\n\n");

    // Check SSL certificates
    if (access(cert_path, F_OK) != 0 || access(key_path, F_OK) != 0) {
        fprintf(stderr, "⚠️  Warning: SSL certificates not found!\n");
        fprintf(stderr, "Generate them with:\n");
        fprintf(stderr, "openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes\n\n");
        fprintf(stderr, "Running without SSL is not recommended (microphone won't work on most phones)\n\n");
        cert_path = NULL;
        key_path = NULL;
    }

    // Setup context creation info
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.mounts = &mount;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT |
                   LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    if (cert_path && key_path) {
        info.ssl_cert_filepath = cert_path;
        info.ssl_private_key_filepath = key_path;
    }

    // Create context
    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create libwebsockets context\n");
        cleanup_vosk_model();
        return 1;
    }

    printf("Server started on %s://0.0.0.0:%d\n", cert_path ? "https" : "http", port);
    printf("Connect to this address from your phone's browser.\n\n");
    printf("Make sure you have:\n");
    printf("1. Generated SSL certificates (cert.pem and key.pem)\n");
    printf("2. Downloaded and extracted a Vosk model to the 'model' folder\n");
    printf("3. Installed required libraries: libwebsockets, vosk-api\n\n");
    printf("Press Ctrl+C to stop the server\n\n");

    // Main event loop
    while (!interrupted) {
        lws_service(context, 100);
    }

    printf("\nShutting down server...\n");

    // Cleanup
    lws_context_destroy(context);
    cleanup_vosk_model();

    printf("Server stopped.\n");
    return 0;
}
