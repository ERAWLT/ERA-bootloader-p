import re

def convert_to_bytes(size):
    units = {'B': 1, 'KB': 1024, 'MB': 1024**2, 'GB': 1024**3}
    number, unit = size.split()
    return int(number) * units[unit]

with open('memory_info.txt', 'r') as file:
    lines = file.readlines()

lines = lines[1:]

modified_lines = []
sum_ram = 0
for line in lines:
    columns = re.split(r'\s{2,}', line.strip())
    size_bytes = convert_to_bytes(columns[1])
    if "FLASH" not in columns[0]:
        sum_ram += size_bytes
    modified_line = f"{columns[0]}  {columns[1]}  {columns[3]}\n"
    modified_lines.append(modified_line)

modified_lines.append(f"RAM: {sum_ram} B\n")

keywords_to_remove = ['DTCMRAM:', 'RAM_D1:', 'RAM_D1_RTOS_HEAP:', 'RAM_D2_RTOS_HEAP:', 'RAM_D3_RTOS_HEAP:', 'ITCMRAM:']
modified_lines = [line for line in modified_lines if not any(keyword in line for keyword in keywords_to_remove)]

with open('memory_info_modified.txt', 'w') as file:
    file.writelines(modified_lines)

