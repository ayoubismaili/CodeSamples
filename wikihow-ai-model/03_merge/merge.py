import os
import torch
from peft import PeftModel
from transformers import AutoModelForCausalLM, AutoTokenizer

# -----------------------------
# PATH CONFIG
# -----------------------------
# Base model (original weights)
base_model_path = "/opt/wikihow-ai-model/pretrained"
# Trained adapters (LoRA output)
adapter_path = "/opt/wikihow-ai-model/output"
# Target directory for GGUF conversion
merged_model_path = "/opt/wikihow-ai-model/merged_model"

# -----------------------------
# 1. LOAD BASE MODEL
# -----------------------------
print("Loading base model (FP16)...")
# We load in FP16 to match SmolLM2's native precision and save RAM
base_model = AutoModelForCausalLM.from_pretrained(
    base_model_path, 
    local_files_only=True,
    torch_dtype=torch.float16, 
    device_map="cpu"  # CPU is more stable for the merge math
)

# -----------------------------
# 2. LOAD LORA ADAPTERS
# -----------------------------
print("Loading LoRA adapters...")
model = PeftModel.from_pretrained(base_model, adapter_path)

# -----------------------------
# 3. MERGE AND UNLOAD
# -----------------------------
print("Merging weights (baking adapters into base)...")
# This creates a standard Llama-architecture model
model = model.merge_and_unload()

# -----------------------------
# 4. SAVE MERGED MODEL
# -----------------------------
print(f"Saving merged model weights to {merged_model_path}...")
model.save_pretrained(merged_model_path)

# -----------------------------
# 5. FIXED TOKENIZER SECTION
# -----------------------------
print("Repairing and saving tokenizer...")
# We pull the clean config from the HF Hub to fix the 'TokenizersBackend' error
# This ensures llama.cpp sees the correct LlamaTokenizer class
tokenizer = AutoTokenizer.from_pretrained(
    "HuggingFaceTB/SmolLM2-135M-Instruct",
    trust_remote_code=True
)

# Ensure pad token matches EOS (standard for SmolLM2/Llama)
if tokenizer.pad_token is None:
    tokenizer.pad_token = tokenizer.eos_token

tokenizer.save_pretrained(merged_model_path)

# -----------------------------
# CLEANUP
# -----------------------------
del model
del base_model
if torch.cuda.is_available():
    torch.cuda.empty_cache()

print(f"✅ Success! Your model in '{merged_model_path}' is ready for GGUF conversion.")
