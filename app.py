
import json
import vosk
from flask import Flask, render_template
from flask_sock import Sock

# --- Configuration ---
MODEL_PATH = "model"
SAMPLE_RATE = 16000

# --- Flask App Initialization ---
app = Flask(__name__)
sock = Sock(app)

# --- Vosk Model Loading ---
print("Loading Vosk model...")
try:
    model = vosk.Model(MODEL_PATH)
except Exception as e:
    print(f"Error loading model: {e}")
    print("Please make sure the 'model' folder is in the same directory and contains the Vosk model files.")
    exit(1)
print("Model loaded successfully.")

# --- Flask Routes ---
@app.route('/')
def index():
    """Serves the main HTML page."""
    return render_template('index.html')

@sock.route('/audio')
def audio_socket(ws):
    """Handles the WebSocket connection for audio streaming."""
    print("Client connected.")
    recognizer = vosk.KaldiRecognizer(model, SAMPLE_RATE)
    recording = False

    try:
        while True:
            message = ws.receive()

            if isinstance(message, str):
                if message == 'start':
                    recording = True
                    print("\n--- Recording Started ---")
                elif message == 'stop':
                    recording = False
                    print("\n--- Recording Stopped ---")
                    final_result = json.loads(recognizer.FinalResult())
                    if final_result.get('text'):
                        final_text = final_result['text']
                        print(f"Final: {final_text}\n")
                        ws.send(json.dumps({'type': 'final', 'text': final_text}))

            elif isinstance(message, bytes) and recording:
                if recognizer.AcceptWaveform(message):
                    result = json.loads(recognizer.Result())
                    if result.get('text'):
                        final_text = result['text']
                        print(f"\nFinal: {final_text}")
                        ws.send(json.dumps({'type': 'final', 'text': final_text}))
                else:
                    partial_result = json.loads(recognizer.PartialResult())
                    if partial_result.get('partial'):
                        partial_text = partial_result['partial']
                        print(f"Partial: {partial_text}", end='\r')
                        ws.send(json.dumps({'type': 'partial', 'text': partial_text}))

    except Exception as e:
        print(f"An error occurred or client disconnected: {e}")
    finally:
        print("Client disconnected.")

# --- Main Execution ---
if __name__ == '__main__':
    print("Starting server on https://0.0.0.0:5000")
    print("Connect to this address from your phone's browser.")
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False,
            ssl_context=('cert.pem', 'key.pem'))
templates/index.html
html<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Real-Time Voice Transcription</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        button {
            width: 100%;
            padding: 15px;
            font-size: 18px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            margin: 10px 0;
        }
        #startBtn {
            background-color: #4CAF50;
            color: white;
        }
        #stopBtn {
            background-color: #f44336;
            color: white;
            display: none;
        }
        #status {
            text-align: center;
            padding: 10px;
            margin: 20px 0;
            border-radius: 5px;
            font-weight: bold;
        }
        .status-ready { background-color: #e3f2fd; color: #1976d2; }
        .status-recording { background-color: #ffebee; color: #c62828; }
        #transcript {
            min-height: 200px;
            padding: 15px;
            border: 2px solid #ddd;
            border-radius: 5px;
            background-color: #fafafa;
            margin-top: 20px;
        }
        .partial { color: #888; font-style: italic; }
        .final { color: #000; font-weight: normal; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎤 Real-Time Voice Transcription</h1>
        
        <div id="status" class="status-ready">Ready</div>
        
        <button id="startBtn">Start Recording</button>
        <button id="stopBtn">Stop Recording</button>
        
        <div id="transcript">
            <p style="color: #888;">Your transcription will appear here...</p>
        </div>
    </div>

    <script>
        const startBtn = document.getElementById('startBtn');
        const stopBtn = document.getElementById('stopBtn');
        const status = document.getElementById('status');
        const transcript = document.getElementById('transcript');
        
        let ws;
        let mediaRecorder;
        let audioContext;
        
        startBtn.addEventListener('click', startRecording);
        stopBtn.addEventListener('click', stopRecording);
        
        async function startRecording() {
            try {
                const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
                ws = new WebSocket(`${protocol}//${window.location.host}/audio`);
                
                ws.onopen = async () => {
                    console.log('WebSocket connected');
                    
                    const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
                    
                    audioContext = new AudioContext({ sampleRate: 16000 });
                    const source = audioContext.createMediaStreamSource(stream);
                    const processor = audioContext.createScriptProcessor(4096, 1, 1);
                    
                    source.connect(processor);
                    processor.connect(audioContext.destination);
                    
                    processor.onaudioprocess = (e) => {
                        if (ws.readyState === WebSocket.OPEN) {
                            const audioData = e.inputBuffer.getChannelData(0);
                            const pcmData = convertFloat32ToInt16(audioData);
                            ws.send(pcmData);
                        }
                    };
                    
                    ws.send('start');
                    
                    startBtn.style.display = 'none';
                    stopBtn.style.display = 'block';
                    status.textContent = 'Recording...';
                    status.className = 'status-recording';
                    transcript.innerHTML = '';
                };
                
                ws.onmessage = (event) => {
                    const data = JSON.parse(event.data);
                    
                    if (data.type === 'partial') {
                        updateTranscript(data.text, false);
                    } else if (data.type === 'final') {
                        updateTranscript(data.text, true);
                    }
                };
                
                ws.onerror = (error) => {
                    console.error('WebSocket error:', error);
                };
                
            } catch (error) {
                console.error('Error starting recording:', error);
                alert('Error accessing microphone: ' + error.message);
            }
        }
        
        function stopRecording() {
            if (ws) {
                ws.send('stop');
                ws.close();
            }
            if (audioContext) {
                audioContext.close();
            }
            
            startBtn.style.display = 'block';
            stopBtn.style.display = 'none';
            status.textContent = 'Ready';
            status.className = 'status-ready';
        }
        
        function updateTranscript(text, isFinal) {
            if (isFinal) {
                const p = document.createElement('p');
                p.className = 'final';
                p.textContent = text;
                transcript.appendChild(p);
                transcript.scrollTop = transcript.scrollHeight;
            } else {
                let partialElement = transcript.querySelector('.partial');
                if (!partialElement) {
                    partialElement = document.createElement('p');
                    partialElement.className = 'partial';
                    transcript.appendChild(partialElement);
                }
                partialElement.textContent = text;
            }
        }
        
        function convertFloat32ToInt16(buffer) {
            const l = buffer.length;
            const buf = new Int16Array(l);
            for (let i = 0; i < l; i++) {
                buf[i] = Math.max(-32768, Math.min(32767, buffer[i] * 32768));
            }
            return buf.buffer;
        }
    </script>
</body>
</html>
