#!/usr/bin/env python3
"""Compare an exported simulator state against JPL Horizons. Standard library only.
Run explicitly: this script makes eight HTTPS requests and retains responses.
It reports measured differences, not a guarantee of accuracy over 1800-2050.
"""
import argparse
import csv
import json
import math
from pathlib import Path
import re
import sys
import urllib.parse
import urllib.request


def vector_from_response(text):
    block = text.split('$$SOE', 1)[1].split('$$EOE', 1)[0]
    values = []
    for axis in ('X', 'Y', 'Z'):
        match = re.search(r'\b' + axis + r'\s*=\s*([+\-\d.Ee]+)', block)
        if not match:
            raise ValueError('Missing coordinate ' + axis)
        values.append(float(match.group(1)))
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('state_csv', type=Path)
    parser.add_argument('--output', type=Path, default=Path('horizons_comparison'))
    parser.add_argument('--cached', action='store_true', help='Use saved replies without network')
    args = parser.parse_args()
    rows = list(csv.DictReader(args.state_csv.open()))
    ids = dict(Mercury=1, Venus=2, Earth=3, Mars=4, Jupiter=5, Saturn=6, Uranus=7, Neptune=8)
    args.output.mkdir(parents=True, exist_ok=True)
    results = []
    for row in rows:
        name = row['name']
        if name == 'Sun':
            continue
        jd = float(row['jd_tdb'])
        if not math.isfinite(jd):
            raise ValueError('Nonfinite date')
        settings = dict(format='json', COMMAND=str(ids[name]), OBJ_DATA='NO',
                        MAKE_EPHEM='YES', EPHEM_TYPE='VECTORS', CENTER='500@10',
                        TLIST=str(jd), TIME_TYPE='TDB', OUT_UNITS='AU-D',
                        REF_PLANE='ECLIPTIC', REF_SYSTEM='ICRF', VEC_TABLE='2',
                        VEC_CORR='NONE')
        url = 'https://ssd.jpl.nasa.gov/api/horizons.api?' + urllib.parse.urlencode(settings)
        cache = args.output / f'{name}_{jd:.5f}.json'
        if args.cached:
            payload = json.loads(cache.read_text())
        else:
            with urllib.request.urlopen(url, timeout=45) as response:
                payload = json.load(response)
            cache.write_text(json.dumps(payload, indent=2))
            cache.with_suffix('.url.txt').write_text(url + '\n')
        if 'error' in payload:
            raise RuntimeError(payload['error'])
        ref = vector_from_response(payload['result'])
        sim = [float(row[f'{axis}_ecliptic_au']) for axis in 'xyz']
        norm = lambda v: math.sqrt(sum(x*x for x in v))
        delta_km = norm([s-r for s, r in zip(sim, ref)]) * 149597870.7
        radial_km = (norm(sim)-norm(ref)) * 149597870.7
        cosine = sum(s*r for s, r in zip(sim, ref))/(norm(sim)*norm(ref))
        angular = math.degrees(math.acos(max(-1, min(1, cosine))))*3600
        results.append(dict(name=name, jd_tdb=jd, position_error_km=delta_km,
                            radial_error_km=radial_km, angular_error_arcsec=angular))
        print(f'{name:8}  position {delta_km:12.3f} km   angular {angular:9.3f} arcsec')
    if len(results) != 8:
        raise ValueError('Expected all eight planetary rows')
    with (args.output/'comparison.csv').open('w', newline='') as output:
        writer = csv.DictWriter(output, fieldnames=results[0])
        writer.writeheader(); writer.writerows(results)
    print('Saved comparison and raw source responses to', args.output)


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print('Comparison failed:', error, file=sys.stderr)
        sys.exit(1)
