import pandas as pd
import time
import platform
from datetime import datetime
from sympy import nsimplify, sympify
from sympy.printing.mathematica import mathematica_code

# Input and output file names
name = 'Benchmark_CASIO_HL-815L_K7_L2'
input_file = name + '.csv'
output_file = name + '_results.csv'
metadata_file = name + '_metadata.txt'

# Read the CSV data using pandas, WARNING: automatic datatype detection!
#data = pd.read_csv(input_file)
data = pd.read_csv(input_file, dtype={'FloatString': str})

# Start the timer
start_time = time.time()

# Process each row
data['nsimplify'] = None
data['nsimplifyMma'] = None
for index, row in data.iterrows():
    try:
        # Convert the Float128 column to a float
        float_value = sympify(row['FloatString'].replace(' I', 'j'))
        # Apply nsimplify
        simplified = nsimplify(float_value)
        # Store the result in new columns
        data.at[index, 'nsimplify'] = str(simplified)
        # Convert the simplified expression to Mathematica code
        mathematica_expr = mathematica_code(simplified)
        data.at[index, 'nsimplifyMma'] = mathematica_expr
    except Exception as e:
        data.at[index, 'nsimplify'] = f"Error: {e}"
        data.at[index, 'nsimplifyMma'] = f"Error: {e}"

# Retain only the original 'Float128' column and the new columns
data = data[['Float128', 'Mathematica', 'nsimplify', 'nsimplifyMma']]

# End the timer
end_time = time.time()
total_time = end_time - start_time

# Metadata
metadata = {
    'Benchmark Name': name,
    'Run Date': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
    'Total Runtime (s)': f"{total_time:.6f}",
    'System Info': platform.platform(),
    'Processor': platform.processor()
}

# Write the metadata to a separate text file
with open(metadata_file, 'w') as f:
    for key, value in metadata.items():
        f.write(f"{key}: {value}\n")

# Write the results to a CSV file
data.to_csv(output_file, index=False)

print(f"Total time for nsimplify(): {total_time:.6f} seconds")
print(f"Results written to {output_file}")
print(f"Metadata written to {metadata_file}")