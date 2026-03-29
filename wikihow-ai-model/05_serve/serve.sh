/opt/llama.cpp/llama.cpp/build/bin/llama-server \
  -m /opt/wikihow-ai-model/wikihow-ai-model.gguf \
  --host 0.0.0.0 \
  --port 8080 \
  -c 2048 \
  --n-gpu-layers 0
