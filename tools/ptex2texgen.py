#!/usr/bin/env python3
"""ptex2texgen — converts Material Maker .ptex projects to TEXGEN JSON.

NOTE: the canonical converter now lives in lib/PtexImport.cpp (used by the
editor's "Import MM.." button and by texgen_render, which accepts .ptex
directly). This script is kept for scripting/CI convenience and may lag
behind the C++ implementation.

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
    'transform': ['In', None, None, None, None, None],
    'shape': [None, None],
    'combine': ['R', 'G', 'B', 'A'],
    'decompose': ['In'],
    'invert': ['In'],
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
    'transform': ['Out'],
    'shape': ['Out'],
    'pattern': ['Out'],
    'bricks': ['Out'],
    'combine': ['Out'],
    'decompose': ['R', 'G', 'B', 'A'],
    'invert': ['Out'],
    'uniform_greyscale': ['Out'],
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


def conv_transform(p):
    return 'Transform2D', {
        'tx': float(p.get('translate_x', 0.0)),
        'ty': float(p.get('translate_y', 0.0)),
        'rot': float(p.get('rotate', 0.0)),
        'scaleX': float(p.get('scale_x', 1.0)),
        'scaleY': float(p.get('scale_y', 1.0)),
        'repeat': bool(p.get('repeat', False)),
    }


def conv_shape(p):
    return 'Shape', {
        'widthIdx': 3, 'heightIdx': 3,
        'shape': int(p.get('shape', 0)),
        'sides': float(p.get('sides', 3.0)),
        'radius': float(p.get('radius', 1.0)),
        'edge': float(p.get('edge', 0.2)),
    }


def conv_pattern(p):
    return 'Pattern', {
        'widthIdx': 3, 'heightIdx': 3,
        'mix': int(p.get('mix', 0)),
        'xWave': int(p.get('x_wave', 0)),
        'xScale': float(p.get('x_scale', 4.0)),
        'yWave': int(p.get('y_wave', 0)),
        'yScale': float(p.get('y_scale', 4.0)),
    }


def conv_uniform_greyscale(p):
    val = float(p.get('color', 0.5))
    return 'Color', {'widthIdx': 3, 'heightIdx': 3,
                     'r': val, 'g': val, 'b': val, 'a': 1.0}


def conv_passthrough_combine(p):
    return 'Combine', {}


def conv_passthrough_decompose(p):
    return 'Decompose', {}


def conv_passthrough_invert(p):
    return 'Invert', {}


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
    'transform': conv_transform,
    'shape': conv_shape,
    'pattern': conv_pattern,
    'uniform_greyscale': conv_uniform_greyscale,
    'combine': conv_passthrough_combine,
    'decompose': conv_passthrough_decompose,
    'invert': conv_passthrough_invert,
}


def convert_graph(mm_nodes, mm_conns, basename, skipped, dyn_out_ports):
    """Convert a list of MM nodes/connections. Returns
    (nodes_out, conns_out, name_to_id, albedo_source)."""
    nodes_out = []
    conns_out = []
    name_to_id = {}
    next_id = [0]

    def alloc():
        i = next_id[0]
        next_id[0] += 1
        return i

    for n in mm_nodes:
        mm_type = n.get('type', '?')
        name = n.get('name', '?')
        pos = n.get('node_position', {})
        params = n.get('parameters', {})

        if mm_type == 'material':
            type_name, p = conv_material(params, basename)
        elif mm_type == 'comment':
            col = n.get('color', {})
            type_name = 'Comment'
            p = {'text': n.get('text', ''),
                 'color': [col.get('r', 0.3), col.get('g', 0.3),
                           col.get('b', 0.3)]}
        elif mm_type == 'graph':
            type_name, p, ports_in, ports_out = graph_to_subgraph(
                n, basename, skipped)
            dyn_out_ports[name] = (ports_in, ports_out)
        elif mm_type in CONVERTERS:
            type_name, p = CONVERTERS[mm_type](params)
        elif mm_type in ('remote', 'debug'):
            # Remotes only drive params whose current values are already
            # baked into each node's parameters — safe to drop silently.
            continue
        else:
            skipped.setdefault(mm_type, []).append(name)
            continue

        nid = alloc()
        name_to_id[name] = (nid, mm_type)
        nodes_out.append({
            'id': nid,
            'typeName': type_name,
            'posX': float(pos.get('x', 0.0)),
            'posY': float(pos.get('y', 0.0)),
            'params': p,
        })

    albedo_source = None
    for c in mm_conns:
        src = name_to_id.get(c.get('from'))
        dst = name_to_id.get(c.get('to'))
        if not src or not dst:
            continue
        from_id, from_type = src
        to_id, to_type = dst
        fp, tp = c.get('from_port', 0), c.get('to_port', 0)

        if from_type == 'graph':
            outs = dyn_out_ports.get(c.get('from'), ([], []))[1]
        else:
            outs = PORTS_OUT.get(from_type, ['Out'])
        if to_type == 'graph':
            ins = dyn_out_ports.get(c.get('to'), ([], []))[0]
        else:
            ins = PORTS_IN.get(to_type, [])

        if fp >= len(outs) or outs[fp] is None or tp >= len(ins) or \
           ins[tp] is None:
            print(f'  ! conexao ignorada: {c} (porta sem mapeamento)')
            continue
        conns_out.append({'fromId': from_id, 'fromSlot': outs[fp],
                          'toId': to_id, 'toSlot': ins[tp]})
        if to_type == 'material':
            # preview priority: Albedo > Height > Normal > first connected
            prio = {'Albedo': 3, 'Height': 2, 'Normal': 1}
            cur = prio.get(ins[tp], 0)
            if albedo_source is None or cur > albedo_source[2]:
                albedo_source = (from_id, outs[fp], cur)

    return nodes_out, conns_out, name_to_id, albedo_source


def graph_to_subgraph(n, basename, skipped):
    """Convert an MM 'graph' node into a TEXGEN Subgraph node.
    Returns (typeName, params, in_port_names, out_port_names)."""
    inner_nodes = [x for x in n.get('nodes', []) if x.get('type') != 'ios']
    inner_conns = n.get('connections', [])
    gen_in = next((x for x in n.get('nodes', [])
                   if x.get('name') == 'gen_inputs'), {})
    gen_out = next((x for x in n.get('nodes', [])
                    if x.get('name') == 'gen_outputs'), {})
    in_ports = [p.get('name', f'in{i}')
                for i, p in enumerate(gen_in.get('ports', []))]
    out_ports = [p.get('name', f'out{i}')
                 for i, p in enumerate(gen_out.get('ports', []))]

    dyn = {}
    real_conns = [c for c in inner_conns
                  if c.get('from') != 'gen_inputs' and
                  c.get('to') != 'gen_outputs']
    nodes_out, conns_out, name_to_id, _ = convert_graph(
        inner_nodes, real_conns, basename, skipped, dyn)

    # boundary connections -> port declarations
    inputs = [{'name': pn, 'targets': []} for pn in in_ports]
    outputs = [{'name': pn, 'id': -1, 'slot': 'Out'} for pn in out_ports]
    for c in inner_conns:
        if c.get('from') == 'gen_inputs':
            k = c.get('from_port', 0)
            dst = name_to_id.get(c.get('to'))
            if k >= len(inputs) or not dst:
                continue
            to_id, to_type = dst
            ins = (dyn.get(c.get('to'), ([], []))[0]
                   if to_type == 'graph' else PORTS_IN.get(to_type, []))
            tp = c.get('to_port', 0)
            if tp < len(ins) and ins[tp]:
                inputs[k]['targets'].append([to_id, ins[tp]])
        elif c.get('to') == 'gen_outputs':
            k = c.get('to_port', 0)
            src = name_to_id.get(c.get('from'))
            if k >= len(outputs) or not src:
                continue
            from_id, from_type = src
            outs = (dyn.get(c.get('from'), ([], []))[1]
                    if from_type == 'graph' else
                    PORTS_OUT.get(from_type, ['Out']))
            fp = c.get('from_port', 0)
            if fp < len(outs) and outs[fp]:
                outputs[k]['id'] = from_id
                outputs[k]['slot'] = outs[fp]

    params = {
        'title': n.get('label', 'Subgraph'),
        'graph': {'nodes': nodes_out, 'connections': conns_out},
        'inputs': inputs,
        'outputs': outputs,
    }
    return 'Subgraph', params, in_ports, out_ports


def convert(ptex, basename):
    skipped = {}
    dyn = {}
    nodes_out, conns_out, name_to_id, albedo_source = convert_graph(
        ptex.get('nodes', []), ptex.get('connections', []), basename,
        skipped, dyn)

    # add an Output node (TEXGEN's render sink) fed by the albedo chain
    if albedo_source:
        out_id = max((n['id'] for n in nodes_out), default=-1) + 1
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
