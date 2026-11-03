"""Create a clean Bandicuss settings partition; never reads or flashes a device.

Requires Espressif's esp-idf-nvs-partition-gen package. The output replaces the
entire NVS partition, so it is for an intentional gift/reset workflow only.
Contains only appcfg7; no Wi-Fi, cache, tokens, or private release material.
"""
import argparse, csv, struct, subprocess, sys
from pathlib import Path

def settings_blob():
    # ESP32 AppConfig v0.7: 80 bytes, little endian; see AppSettings.h.
    # Name, empty city, neutral coordinates, no auto-rotation, six enabled pages,
    # Central / 12h / Fahrenheit, no location, Weather Cloud / fixed / BGR.
    value=struct.pack('<I19s33sffH6B8B',0x44420701,b'Bandicuss',b'',0,0,0,
                      5,0,1,2,3,4,6,0,0,1,0,0,0,0)
    assert len(value)==80
    return value

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir',type=Path,required=True)
    args=parser.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=True)
    blob=out/'bandicuss-appcfg7.bin';blob.write_bytes(settings_blob())
    source=out/'bandicuss-nvs.csv'
    with source.open('w',newline='',encoding='utf8') as f:
        csv.writer(f).writerows([['key','type','encoding','value'],['deskbuddy','namespace','',''],['appcfg7','file','binary',str(blob)]])
    subprocess.run([sys.executable,'-m','esp_idf_nvs_partition_gen','generate',str(source),str(out/'Bandicuss-settings.bin'),'0x5000'],check=True)
    assert (out/'Bandicuss-settings.bin').stat().st_size==0x5000
    print('Created clean Bandicuss settings (no Wi-Fi or weather cache). Nothing flashed.')
if __name__=='__main__':main()
