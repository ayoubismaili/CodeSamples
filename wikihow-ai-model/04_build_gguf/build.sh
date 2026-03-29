
# Clone the repository
git clone https://github.com/ggml-org/llama.cpp

# Install conversion dependencies
pip install -r llama.cpp/requirements.txt

# Convert the merged model to GGUF
python llama.cpp/convert_hf_to_gguf.py /opt/wikihow-ai-model/merged_model \
  --outfile /opt/wikihow-ai-model/wikihow-ai-model.gguf \
  --outtype f16
