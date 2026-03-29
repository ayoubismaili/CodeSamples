apt-get update -y

apt-get install python3.10 python3.10-dev python3.10-distutils libpython3.10-dev python3-venv python3-pip -y

/usr/bin/python3.10 -m venv /opt/wikihow-ai-model/00_venv

echo "Now source /opt/wikihow-ai-model/00_venv/bin/activate"
echo "Then pip install transformers datasets peft accelerate torch"
