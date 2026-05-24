import json
import gzip
import struct
import sys
import numpy as np
from usearch.index import Index, Matches

#Script para conversão do arquivo de referências em JSON para um arquivo index do USearch e outro com as labels de cada registro

def convert_json_to_bin(json_path, bin_path):

    LABEL_MAP = {
        "legit": 0,
        "fraud": 1
    }

    index = Index(
                ndim=14, # Define the number of dimensions in input vectors
                metric='cos', # Choose 'l2sq', 'ip', 'haversine' or other metric, default = 'cos'
                dtype='f32', # Quantize to 'f16', 'e5m2', 'e4m3', 'e3m2', 'e2m3', 'u8', 'i8', 'b1'..., default = None
                expansion_add=0, # Control the recall of indexing, optional
                expansion_search=0, # Control the quality of search, optional
                )


    print(f"Reading from {json_path}...")
    with gzip.open(json_path, 'rt', encoding='utf-8') as f:
        data = json.load(f)

    with open(bin_path, "wb") as f:


        i = 0

        for record in data:

            print(f"Record {i}")


            label_int = LABEL_MAP.get(record.get('label'), 0)

            vector = np.array(record["vector"])

            index.add(i, vector)

            f.write(struct.pack('<i', label_int))

            i += 1

            
    index.save('index.usearch')

    

            
    print(f"Done. Wrote {len(data)} records.")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python create_index.py <input.json or input.json.gz> <output.bin>")
        sys.exit(1)
        
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    convert_json_to_bin(input_file, output_file)
