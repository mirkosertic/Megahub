Import("env")
import os
import gzip

data_dir = os.path.join(env.get("PROJECT_DIR"), "data/web")
output_dir = os.path.join(env.subst("$BUILD_DIR"), "generated")

os.makedirs(output_dir, exist_ok=True)

header_content = '#pragma once\n#include <Arduino.h>\n\n'

for root, dirs, files in os.walk(data_dir):
    for file in files:
        if file.endswith('.gz'):
            continue
            
        filepath = os.path.join(root, file)
        rel_path = os.path.relpath(filepath, data_dir)
        var_name = rel_path.replace('/', '_').replace('.', '_').replace('-', '_')
        
        # Read and compress
        with open(filepath, 'rb') as f:
            content = f.read()
        compressed = gzip.compress(content, compresslevel=9)
        
        # Generate C array
        header_content += f'// {rel_path} ({len(content)} -> {len(compressed)} bytes)\n'
        header_content += f'const uint8_t {var_name}_gz[] PROGMEM = {{\n'
        
        # Write bytes in rows of 16
        for i in range(0, len(compressed), 16):
            chunk = compressed[i:i+16]
            header_content += '  ' + ', '.join(f'0x{b:02x}' for b in chunk) + ',\n'
        
        header_content += '};\n'
        header_content += f'const size_t {var_name}_gz_len = {len(compressed)};\n\n'

# Write header file
with open(os.path.join(output_dir, 'embedded_files.h'), 'w') as f:
    f.write(header_content)

print("Embedded files generated at " + output_dir)
