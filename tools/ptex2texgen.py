#!/usr/bin/env python3
"""ptex2texgen — converts Material Maker .ptex projects to TEXGEN JSON.

Usage: ptex2texgen.py input.ptex output.json [--basename NAME]

Node types without a mapping are reported and skipped (along with their
connections) — the report doubles as a backlog of nodes to port next.
"""
import argparse
import json
import sys

# ============================================================
# Mapping tables: MM node type -> TEXGEN typeName + param/port maps
# ============================================================

# input/output slot names by MM port index
PORTS_IN = {
    'colorize': ['In'],
    'blend': ['A', 'B', 'Mask'],
    'warp': ['In', 'Height'],
    'normal_map': ['Height'],
    'material': ['Albedo', 'Metallic', 'Roughness', 'Emission', 'Normal',
                 'AO', 'Height'],
}
PORTS_OUT = {
    'perlin': ['Out'],
    'fbm': ['Out'],
    'uniform': ['Out'],
    'colorize': ['Out'],
    'blend': ['Out'],
    'warp': ['Out'],
    'normal_map': ['Out'],
    # MM voronoi ports: 0=Nodes(F1), 1=Borders(Edge), 2=Random color, 3=Fill
    'voronoi': ['F1', 'Edge', 'Color', None],
}

# MM fbm.mmg noise variant -> FBM mode enum
FBM_NOISE = {'value': 0, 'perlin': 1, 'perlinabs': 2,
             'cellular': 3, 'cellular2': 4, 'cellular3': 5,
             'cellular4': 6, 'cellular5': 7, 'cellular6': 8}

# MM bricks pattern -> MMBricksPattern enum
BRICKS_PATTERN = {'rb': 0, 'rb2': 1, 'hb': 2, 'bw': 3, 'sb': 4}


def gradient_to_stops(grad):
    stops = []
    for p in grad.get('points', []):
        stops.append([p.get('pos', 0.0), p.get('r', 0.0), p.get('g', 0.0),
                      p.get('b', 0.0), p.get('a', 1.0)])
    stops.sort(key=lambda s: s[0])
    return stops or [[0, 0, 0, 0, 1], [1, 1, 1, 1, 1]]


def conv_perlin(p):
    # MM 'perlin' is octaved value noise -> FBM mode 0 (Value)
    return 'FBM', {
        'widthIdx': 3, 'heightIdx': 3, 'mode': 0,
        'scaleX': int(round(p.get('scale_x', 4))),
        'scaleY': int(round(p.get('scale_y', 4))),
        'folds': 0,
        'octaves': int(round(p.get('iterations', 3))),
        'persistence': float(p.get('persistence', 0.5)),
        'seed': float(p.get('seed', 0.0)),
    }


def conv_fbm(p):
    return 'FBM', {
        'widthIdx': 3, 'heightIdx': 3,
        'mode': FBM_NOISE.get(p.get('noise', 'perlin'), 1),
        'scaleX': int(round(p.get('scale_x', 4))),
        'scaleY': int(round(p.get('scale_y', 4))),
        'folds': int(round(p.get('folds', 0))),
        'octaves': int(round(p.get('iterations', 3))),
        'persistence': float(p.get('persistence', 0.5)),
        'seed': float(p.get('seed', 0.0)),
    }


def conv_voronoi(p):
    return 'Voronoi', {
        'widthIdx': 3, 'heightIdx': 3,
        'scaleX': int(round(p.get('scale_x', 4))),
        'scaleY': int(round(p.get('scale_y', 4))),
        'stretchX': float(p.get('stretch_x', 1.0)),
        'stretchY': float(p.get('stretch_y', 1.0)),
        'intensity': float(p.get('intensity', 0.75)),
        'randomness': float(p.get('randomness', 1.0)),
        'seed': float(p.get('seed', 0.0)),
    }


def conv_colorize(p):
    return 'Colorize', {'stops': gradient_to_stops(p.get('gradient', {}))}


def conv_blend(p):
    # MM blend_type order matches MMBlendMode exactly
    return 'Blend', {
        'mode': int(p.get('blend_type', 0)),
        'opacity': float(p.get('amount', 1.0)),
    }


def conv_warp(p):
    return 'Warp', {
        'amount': float(p.get('amount', 0.1)),
        'epsilon': float(p.get('eps', 0.005)),
    }


def conv_normal_map(p):
    # normal_map.mmg exposes: param0=size, param1=amount, param2=format
    return 'NormalMap', {
        'amount': float(p.get('param1', 1.0)),
        'format': int(p.get('param2', 0)),
    }


def conv_uniform(p):
    col = p.get('color', {})
    return 'Color', {
        'widthIdx': 3, 'heightIdx': 3,
        'r': float(col.get('r', 0.0)), 'g': float(col.get('g', 0.0)),
        'b': float(col.get('b', 0.0)), 'a': float(col.get('a', 1.0)),
    }


def conv_bricks(p):
    return 'BricksMM', {
        'widthIdx': 3, 'heightIdx': 3,
        'pattern': BRICKS_PATTERN.get(p.get('pattern', 'rb'), 0),
        'countX': int(round(p.get('columns', 4))),
        'countY': int(round(p.get('rows', 8))),
        'repeat': int(round(p.get('repeat', 1))),
        'offset': float(p.get('row_offset', 0.5)),
        'mortar': float(p.get('mortar', 0.1)),
        'round': float(p.get('round', 0.1)),
        'bevel': float(p.get('bevel', 0.2)),
        'colorBalance': 0.5,
        'seed': 0.0,
    }


def conv_material(p, basename):
    return 'Material', {'baseName': basename}


CONVERTERS = {
    'perlin': conv_perlin,
    'fbm': conv_fbm,
    'voronoi': conv_voronoi,
    'colorize': conv_colorize,
    'blend': conv_blend,
    'warp': conv_warp,
    'normal_map': conv_normal_map,
    'uniform': conv_uniform,
    'bricks': conv_bricks,
}


def convert(ptex, basename):
    skipped = {}
    nodes_out = []
    conns_out = []
    name_to_id = {}
    next_id = 0

    for n in ptex.get('nodes', []):
        mm_type = n.get('type', '?')
        name = n.get('name', '?')
        pos = n.get('node_position', {})
        params = n.get('parameters', {})

        if mm_type == 'material':
            type_name, p = conv_material(params, basename)
        elif mm_type == 'comment':
            # MM stores comment text/color at node level, not in parameters
            col = n.get('color', {})
            type_name = 'Comment'
            p = {'text': n.get('text', ''),
                 'color': [col.get('r', 0.3), col.get('g', 0.3),
                           col.get('b', 0.3)]}
        elif mm_type in CONVERTERS:
            type_name, p = CONVERTERS[mm_type](params)
        else:
            skipped.setdefault(mm_type, []).append(name)
            continue

        nid = next_id
        next_id += 1
        name_to_id[name] = (nid, mm_type)
        nodes_out.append({
            'id': nid,
            'typeName': type_name,
            'posX': float(pos.get('x', 0.0)),
            'posY': float(pos.get('y', 0.0)),
            'params': p,
        })

    albedo_source = None
    for c in ptex.get('connections', []):
        src = name_to_id.get(c.get('from'))
        dst = name_to_id.get(c.get('to'))
        if not src or not dst:
            continue
        from_id, from_type = src
        to_id, to_type = dst
        outs = PORTS_OUT.get(from_type, ['Out'])
        ins = PORTS_IN.get(to_type, [])
        fp, tp = c.get('from_port', 0), c.get('to_port', 0)
        if fp >= len(outs) or outs[fp] is None or tp >= len(ins):
            print(f'  ! conexao ignorada: {c} (porta sem mapeamento)')
            continue
        conns_out.append({'fromId': from_id, 'fromSlot': outs[fp],
                          'toId': to_id, 'toSlot': ins[tp]})
        if to_type == 'material' and ins[tp] == 'Albedo':
            albedo_source = (from_id, outs[fp])

    # add an Output node (TEXGEN's render sink) fed by the albedo chain
    if albedo_source:
        out_id = next_id
        next_id += 1
        nodes_out.append({'id': out_id, 'typeName': 'Output',
                          'posX': 1200.0, 'posY': 0.0,
                          'params': {'filename': basename + '_preview.png'}})
        conns_out.append({'fromId': albedo_source[0],
                          'fromSlot': albedo_source[1],
                          'toId': out_id, 'toSlot': 'In'})

    return {'nodes': nodes_out, 'connections': conns_out}, skipped


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input')
    ap.add_argument('output')
    ap.add_argument('--basename', default=None)
    args = ap.parse_args()

    base = args.basename
    if not base:
        base = args.input.split('/')[-1].rsplit('.', 1)[0]

    ptex = json.load(open(args.input))
    result, skipped = convert(ptex, base)
    json.dump(result, open(args.output, 'w'), indent=2)

    n_ok = len(result['nodes'])
    print(f'{args.input} -> {args.output}: {n_ok} nos convertidos')
    if skipped:
        print('  tipos sem mapeamento (backlog):')
        for t, names in sorted(skipped.items()):
            print(f'    {t}: {len(names)}x ({", ".join(names[:4])})')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
