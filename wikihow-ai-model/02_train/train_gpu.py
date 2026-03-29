import os
import torch  # Added to handle device movement
import pandas as pd
from datasets import Dataset
from transformers import AutoTokenizer, AutoModelForCausalLM, Trainer, TrainingArguments, DataCollatorForLanguageModeling
from peft import LoraConfig, get_peft_model, TaskType

# -----------------------------
# CONFIG
# -----------------------------
###MODEL_NAME = "EleutherAI/gpt-neo-125M"  # Tiny model, CPU-friendly
MODEL_NAME = "HuggingFaceTB/SmolLM2-135M-Instruct"
DATA_PATH  = "/opt/wikihow-ai-model/01_data"   # Folder with parquet files
PRETRAIN_DIR = "/opt/wikihow-ai-model/pretrained" # Pretrained
OUTPUT_DIR = "/opt/wikihow-ai-model/output" # Where trained model will be saved
EPOCHS = 3
BATCH_SIZE = 4                          # Adjust if you have low RAM
MAX_LENGTH = 256                        # Max tokens per sample

# -----------------------------
# LOAD PARQUET DATA
# -----------------------------
print("Loading Parquet dataset...")
dfs = []
for file in os.listdir(DATA_PATH):
    if file.endswith(".parquet"):
        dfs.append(pd.read_parquet(os.path.join(DATA_PATH, file)))
data = pd.concat(dfs, ignore_index=True)

# Expecting your parquet to have "prompt" and "text" columns
if "prompt" not in data.columns or "text" not in data.columns:
    raise ValueError("Parquet files must have 'prompt' and 'text' columns")

# Combine prompt + text for training
data["text"] = data["prompt"].astype(str) + " " + data["text"].astype(str)

# Convert to Hugging Face Dataset
dataset = Dataset.from_pandas(data[["text"]])

# -----------------------------
# TOKENIZER
# -----------------------------
print("Loading tokenizer...")
if not os.path.exists(PRETRAIN_DIR + "/tokenizer.json"):
    tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
    tokenizer.save_pretrained(PRETRAIN_DIR)
else:
    tokenizer = AutoTokenizer.from_pretrained(PRETRAIN_DIR, local_files_only=True)

tokenizer.pad_token = tokenizer.eos_token  # GPT-Neo needs this

def tokenize_fn(batch):
    return tokenizer(batch["text"], padding="max_length", truncation=True, max_length=MAX_LENGTH)

tokenized_dataset = dataset.map(tokenize_fn, batched=True)
# Slicing the dataset to 200 samples
tokenized_dataset = tokenized_dataset.select(range(200))

# -----------------------------
# MODEL + LORA
# -----------------------------
print("Loading model and adding LoRA adapters...")
if not os.path.exists(PRETRAIN_DIR + "/model.safetensors"):
    model = AutoModelForCausalLM.from_pretrained(MODEL_NAME)
    model.save_pretrained(PRETRAIN_DIR)
else:
    model = AutoModelForCausalLM.from_pretrained(PRETRAIN_DIR, local_files_only=True)

# --- GPU CHANGE: Move model to GPU ---
model = model.to("cuda") 

lora_config = LoraConfig(
    r=8,
    lora_alpha=16,
    target_modules=["q_proj", "v_proj"],  # GPT-Neo attention modules
###    target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],
    lora_dropout=0.05,
    bias="none",
    task_type=TaskType.CAUSAL_LM
)
model = get_peft_model(model, lora_config)

# -----------------------------
# DATA COLLATOR
# -----------------------------
data_collator = DataCollatorForLanguageModeling(tokenizer, mlm=False)

# -----------------------------
# TRAINING
# -----------------------------
training_args = TrainingArguments(
    output_dir=OUTPUT_DIR,
    num_train_epochs=EPOCHS,
    per_device_train_batch_size=BATCH_SIZE,
    save_strategy="epoch",
    logging_steps=50,
    save_total_limit=2,
    fp16=True,           # --- GPU CHANGE: Set to True ---
#    evaluation_strategy="no",
    remove_unused_columns=True,
    gradient_accumulation_steps=4
)

trainer = Trainer(
    model=model,
    args=training_args,
    train_dataset=tokenized_dataset,
#    tokenizer=tokenizer,
    data_collator=data_collator
)

print("Starting training on GPU (Colab)...")
trainer.train()

# -----------------------------
# SAVE MODEL
# -----------------------------
print(f"Saving trained model to {OUTPUT_DIR}...")
model.save_pretrained(OUTPUT_DIR)
tokenizer.save_pretrained(OUTPUT_DIR)

print("Training complete! You can now convert to GGUF for CPU inference.")