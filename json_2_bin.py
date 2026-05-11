import json
import gzip
import struct
import sys

#Script para conversão do arquivo de referências em JSON para binário

def convert_json_to_bin(json_path, bin_path):
    # Mapping for labels (adjust as necessary)
    LABEL_MAP = {
        "legit": 0,
        "fraud": 1
    }

    # Struct format: 14 floats, 1 signed byte (int8_t), 7 padding bytes
    # '<' for little-endian (standard for most processors)
    # 14f: 14 floats
    # b: 1 signed char (int8_t)
    # 7x: 7 pad bytes
    struct_format = '<14fb7x'
    
    if json_path.endswith('.gz'):
        open_func = gzip.open
    else:
        open_func = open

    print(f"Reading from {json_path}...")
    with open_func(json_path, 'rt', encoding='utf-8') as f:
        data = json.load(f)
    
    print(f"Writing to {bin_path}...")
    with open(bin_path, 'wb') as f:
        for item in data:
            vector = item['vector']
            if len(vector) != 14:
                raise ValueError(f"Expected 14 coordinates, got {len(vector)}")
            
            label_str = item.get('label', '')
            label_int = LABEL_MAP.get(label_str, -1) # Default to -1 if unknown
            
            # Pack the data
            packed_data = struct.pack(struct_format, *vector, label_int)
            f.write(packed_data)
            
    print(f"Done. Wrote {len(data)} records.")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python json_2_bin.py <input.json or input.json.gz> <output.bin>")
        sys.exit(1)
        
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    convert_json_to_bin(input_file, output_file)
