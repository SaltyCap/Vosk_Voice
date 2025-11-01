#ifndef VOSK_SERVER_H
#define VOSK_SERVER_H

#include <libwebsockets.h>
#include <vosk_api.h>
#include <stdint.h>
#include <stdbool.h>

// Configuration
#define MODEL_PATH "model"
#define SAMPLE_RATE 16000
#define DEFAULT_PORT 5000
#define MAX_PAYLOAD_SIZE 65536

// Session state
typedef struct {
    VoskRecognizer *recognizer;
    bool recording;
    char *partial_text;
    char *final_text;
} session_data_t;

// Global Vosk model
extern VoskModel *vosk_model;

// Function declarations
int init_vosk_model(const char *model_path);
void cleanup_vosk_model(void);

session_data_t *create_session(void);
void destroy_session(session_data_t *session);

int handle_audio_data(session_data_t *session, const uint8_t *data, size_t len,
                      char **result_json, bool *is_final);
int start_recording(session_data_t *session);
int stop_recording(session_data_t *session, char **final_result);

#endif // VOSK_SERVER_H
