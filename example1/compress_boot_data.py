import os
import re

Import("env")
env = DefaultEnvironment()
project_dir = env['PROJECT_DIR']
def rle_compress(data):
    compressed = []
    current_value = data[0]
    count = 1
    for value in data[1:]:
        if value == current_value:
            count += 1
        else:
            compressed.append((current_value, count))
            current_value = value
            count = 1
    compressed.append((current_value, count))
    return compressed

def parse_data_file(file_path):
    with open(file_path, 'r') as file:
        data = file.read()
    data = [int(x, 0) for x in re.findall(r'0x[0-9A-Fa-f]+', data)]
    return data

def generate_header_file(directory):
    header_content = "#ifndef BOOT_H\n#define BOOT_H\n\n#include <Arduino.h>\n\n"

    for filename in sorted(os.listdir(directory)):
        if filename.startswith("_boot") and filename.endswith(".dat"):
            base_name = os.path.splitext(filename)[0]
            file_path = os.path.join(directory, filename)
            
            data = parse_data_file(file_path)
            compressed_data = rle_compress(data)

            array_name = f"{base_name}_compressed"
            header_content += f"const uint16_t {array_name}[] PROGMEM = {{\n"
            for value, count in compressed_data:
                header_content += f"    {value}, {count},\n"
            header_content += "};\n\n"

    header_content += "#endif // BOOT_H\n"
    with open( os.path.join(project_dir, "include", "boot.h"), 'w') as header_file:
        header_file.write(header_content)

# Задаем директорию, где находятся файлы данных
data_directory = "bootdata"
generate_header_file(data_directory)