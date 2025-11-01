#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vosk_api.h>
#include "vosk_server.h"

// Global Vosk model
VoskModel *vosk_model = NULL;

/**
 * Initialize the Vosk model
 */
int init_vosk_model(const char *model_path) {
    vosk_set_log_level(-1); // Reduce Vosk logging

    vosk_model = vosk_model_new(model_path);
    if (!vosk_model) {
        return -1;
    }

    return 0;
}

/**
 * Cleanup the Vosk model
 */
void cleanup_vosk_model(void) {
    if (vosk_model) {
        vosk_model_free(vosk_model);
        vosk_model = NULL;
    }
}

/**
 * Create a new session
 */
session_data_t *create_session(void) {
    if (!vosk_model) {
        fprintf(stderr, "Vosk model not initialized\n");
        return NULL;
    }

    session_data_t *session = calloc(1, sizeof(session_data_t));
    if (!session) {
        return NULL;
    }

    session->recognizer = vosk_recognizer_new(vosk_model, SAMPLE_RATE);
    if (!session->recognizer) {
        free(session);
        return NULL;
    }

    // Configure recognizer for speed
    vosk_recognizer_set_max_alternatives(session->recognizer, 0);
    vosk_recognizer_set_words(session->recognizer, 0);

    session->recording = false;
    session->partial_text = NULL;
    session->final_text = NULL;

    return session;
}

/**
 * Destroy a session
 */
void destroy_session(session_data_t *session) {
    if (!session) {
        return;
    }

    if (session->recognizer) {
        vosk_recognizer_free(session->recognizer);
    }

    if (session->partial_text) {
        free(session->partial_text);
    }

    if (session->final_text) {
        free(session->final_text);
    }

    free(session);
}

/**
 * Start recording
 */
int start_recording(session_data_t *session) {
    if (!session) {
        return -1;
    }

    session->recording = true;

    // Reset recognizer
    vosk_recognizer_reset(session->recognizer);

    return 0;
}

/**
 * Stop recording and get final result
 */
int stop_recording(session_data_t *session, char **final_result) {
    if (!session) {
        return -1;
    }

    session->recording = false;

    // Get final result
    const char *result = vosk_recognizer_final_result(session->recognizer);

    if (result && final_result) {
        // Parse JSON to extract text and format response
        // For now, we'll create a simple JSON response
        // In production, use a proper JSON library

        // Find "text" field in JSON
        const char *text_start = strstr(result, "\"text\" : \"");
        if (text_start) {
            text_start += 10; // Skip "text" : "
            const char *text_end = strchr(text_start, '"');
            if (text_end) {
                size_t text_len = text_end - text_start;
                if (text_len > 0) {
                    char *text = strndup(text_start, text_len);
                    if (text) {
                        // Create JSON response
                        asprintf(final_result, "{\"type\":\"final\",\"text\":\"%s\"}", text);
                        printf("Final: %s\n", text);
                        free(text);
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * Handle incoming audio data
 */
int handle_audio_data(session_data_t *session, const uint8_t *data, size_t len,
                      char **result_json, bool *is_final) {
    if (!session || !session->recording) {
        return -1;
    }

    // Feed audio to recognizer
    int accept_result = vosk_recognizer_accept_waveform(session->recognizer, (const char *)data, len);

    if (accept_result) {
        // Final result available
        const char *result = vosk_recognizer_result(session->recognizer);

        if (result) {
            // Parse JSON to extract text
            const char *text_start = strstr(result, "\"text\" : \"");
            if (text_start) {
                text_start += 10; // Skip "text" : "
                const char *text_end = strchr(text_start, '"');
                if (text_end) {
                    size_t text_len = text_end - text_start;
                    if (text_len > 0) {
                        char *text = strndup(text_start, text_len);
                        if (text && strlen(text) > 0) {
                            // Create JSON response
                            asprintf(result_json, "{\"type\":\"final\",\"text\":\"%s\"}", text);
                            printf("\nFinal: %s\n", text);
                            *is_final = true;
                            free(text);
                        }
                    }
                }
            }
        }
    } else {
        // Partial result
        const char *partial = vosk_recognizer_partial_result(session->recognizer);

        if (partial) {
            // Parse JSON to extract partial text
            const char *partial_start = strstr(partial, "\"partial\" : \"");
            if (partial_start) {
                partial_start += 13; // Skip "partial" : "
                const char *partial_end = strchr(partial_start, '"');
                if (partial_end) {
                    size_t partial_len = partial_end - partial_start;
                    if (partial_len > 0) {
                        char *text = strndup(partial_start, partial_len);
                        if (text && strlen(text) > 0) {
                            // Create JSON response
                            asprintf(result_json, "{\"type\":\"partial\",\"text\":\"%s\"}", text);
                            printf("Partial: %s\r", text);
                            fflush(stdout);
                            *is_final = false;
                            free(text);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
