import json
import gzip
import sys
import numpy as np
from usearch.index import Index

def convert_json_to_bin(json_path, bin_path):
    LABEL_MAP = {
        "legit": 0,
        "fraud": 1
    }

    # Inicializa o índice da USearch
    index = Index(
        ndim=14,
        metric='cos',
        dtype='f32',
        expansion_add=16,    
        expansion_search=16
    )

    print(f"Reading from {json_path}...")
    with gzip.open(json_path, 'rt', encoding='utf-8') as f:
        data = json.load(f)

    total_records = len(data)
    print(f"Total de registros: {total_records}. Alocando matrizes...")

    # CRIANDO AS MATRIZES EM MEMÓRIA
    keys = np.arange(total_records, dtype=np.uint64)
    vectors = np.empty((total_records, 14), dtype=np.float32)
    labels = np.empty(total_records, dtype=np.int8) 

    print("Populando as matrizes Python...")
    for i, record in enumerate(data):
        vectors[i] = record["vector"]
        labels[i] = LABEL_MAP.get(record.get('label'), 0)

    # SALVANDO OS LABELS DE UMA VEZ SÓ 
    print(f"Writing labels matrix to {bin_path}...")
    with open(bin_path, "wb") as f_bin:
        f_bin.write(labels.tobytes()) # Transforma a matriz inteira em bytes puros

    # INDEXANDO A MATRIZ INTEIRA NA USEARCH (Batch Add em C++)
    print("Building HNSW graph from matrix...")
    index.add(keys, vectors)
            
    print("Saving index to index.usearch...")
    index.save('index.usearch')
    
    print(f"Done. Processed {total_records} records successfully.")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python create_index.py <input.json.gz> <output_labels.bin>")
        sys.exit(1)
        
    convert_json_to_bin(sys.argv[1], sys.argv[2])