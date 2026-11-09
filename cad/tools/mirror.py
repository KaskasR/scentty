"""Python mirror of scentty_chassis.fs (same formulas, default values) for checking + preview."""
import cadquery as cq

P = dict(t=3, W=560, H=300, ebayW=120, plinthH=35, plinthD=110, cupDepth=8, nBottles=4,
         bottleD=38.1, bottleH=140, bottlePitch=75, bottleX0=55, outX=375, outPadD=45, outD=30,
         outH=80, padH=10, nozDx=30, pumpWallY=150, chaseD=60, pumpZ=200, pumpHoleD=30,
         pumpScrewDx=48.5, faceW=54.5, faceH=40.3, headDepth=26.5, motorL=44, motorD=29, jackD=8,
         swW=19, swH=13, boardDx=62, boardDy=82, bedW=610, bedH=305, fitClear=0.2, flangeW=4, flangeT=2, cupFloor=0.6, tubeOD=5, neckID=12, nozGap=15, funnelD=28, funnelH=20, nStore=4, rackZ=100, barbRoom=20, joints=True, fingerW=20, scrZ=230, scrW=86, scrH=50, winW=58.6, winH=44.2, winOff=0, scrHoleDx=76.08, hasSwitch=False, scrHoleDz=44)


def box(lo, hi):
    return cq.Workplane().box(*(h - l for l, h in zip(lo, hi)), centered=False).translate(lo).val()


def cyl(a, b, r):
    a, b = cq.Vector(*a), cq.Vector(*b)
    return cq.Solid.makeCylinder(r, (b - a).Length, a, (b - a).normalized())


def build(p):
    t, W, H, ph, pd, pwy = p['t'], p['W'], p['H'], p['plinthH'], p['plinthD'], p['pumpWallY']
    xd = W - p['ebayW']; D = pwy + t + p['chaseD'] + t; yc = pd / 2; xb = (xd + W) / 2; e = 1; m3 = 1.6
    n = p['nBottles']; bx = [p['bottleX0'] + i * p['bottlePitch'] for i in range(n)]
    J = t if p['joints'] else 0
    panels = {
        'Base': box((0, 0, 0), (W, D, t)),
        'Left side': box((0, 0, t - J), (t, D, H - t + J)),
        'Right side': box((W - t, 0, t - J), (W, D, H - t + J)),
        'Back (screw-on)': box((t - J, D - t, t - J), (W - t + J, D, H - t + J)),
        'Top (wet bay)': box((0, 0, H - t), (xd + t, D, H)),
        'Bay lid (screw-on)': box((xd + t, 0, H - t), (W, D, H)),
        'Divider wall': box((xd, 0, t - J), (xd + t, D - t + J, H - t + J)),
        'Bay front (QR)': box((xd + t - J, 0, t - J), (W - t + J, t, H - t + J)),
        'Pump wall': box((t - J, pwy, t - J), (xd, pwy + t, H - t + J)),
        'Plinth top': box((t, 0, ph - t), (xd, pd, ph)),
        'Plinth front': box((t - J, 0, t - J), (xd, t, ph - t + J)),
        'Plinth back': box((t - J, pd - t, t - J), (xd, pd, ph - t + J)),
    }
    joint_report = []
    if p['joints']:
        corners = [
            ('Back (screw-on)', (0, D - t, 0), (t, D, t)), ('Back (screw-on)', (W - t, D - t, 0), (W, D, t)),
            ('Back (screw-on)', (0, D - t, H - t), (t, D, H)), ('Back (screw-on)', (W - t, D - t, H - t), (W, D, H)),
            ('Divider wall', (xd, D - t, 0), (xd + t, D, t)), ('Divider wall', (xd, D - t, H - t), (xd + t, D, H)),
            ('Bay front (QR)', (xd, 0, 0), (xd + t, t, t)), ('Bay front (QR)', (W - t, 0, 0), (W, t, t)),
            ('Bay front (QR)', (xd, 0, H - t), (xd + t, t, H)), ('Bay front (QR)', (W - t, 0, H - t), (W, t, H)),
            ('Pump wall', (0, pwy, 0), (t, pwy + t, t)), ('Pump wall', (0, pwy, H - t), (t, pwy + t, H)),
            ('Plinth front', (0, 0, 0), (t, t, t)), ('Plinth front', (0, 0, ph - t), (t, t, ph)),
            ('Plinth back', (0, pd - t, 0), (t, pd, t)), ('Plinth back', (0, pd - t, ph - t), (t, pd, ph))]
        for pn, lo, hi in corners:
            panels[pn] = panels[pn].cut(box(lo, hi))
        L, R, B, T, LID, BK, DV, BF, PW, PT, PF, PB = ('Left side', 'Right side', 'Base', 'Top (wet bay)', 'Bay lid (screw-on)', 'Back (screw-on)',
            'Divider wall', 'Bay front (QR)', 'Pump wall', 'Plinth top', 'Plinth front', 'Plinth back')
        joints = [
            (L, B, (0, 0, 0), (t, D, t), 1), (L, T, (0, 0, H - t), (t, D, H), 1),
            (R, B, (W - t, 0, 0), (W, D, t), 1), (R, LID, (W - t, 0, H - t), (W, D, H), 1),
            (BK, B, (t, D - t, 0), (W - t, D, t), 0), (BK, L, (0, D - t, t), (t, D, H - t), 2),
            (BK, R, (W - t, D - t, t), (W, D, H - t), 2), (BK, T, (t, D - t, H - t), (xd + t, D, H), 0),
            (BK, LID, (xd + t, D - t, H - t), (W - t, D, H), 0),
            (DV, B, (xd, 0, 0), (xd + t, D - t, t), 1), (DV, T, (xd, 0, H - t), (xd + t, D - t, H), 1),
            (DV, BK, (xd, D - t, t), (xd + t, D, H - t), 2),
            (BF, B, (xd + t, 0, 0), (W - t, t, t), 0), (BF, LID, (xd + t, 0, H - t), (W - t, t, H), 0),
            (BF, DV, (xd, 0, t), (xd + t, t, H - t), 2), (BF, R, (W - t, 0, t), (W, t, H - t), 2),
            (PW, B, (t, pwy, 0), (xd, pwy + t, t), 0), (PW, T, (t, pwy, H - t), (xd, pwy + t, H), 0),
            (PW, L, (0, pwy, t), (t, pwy + t, H - t), 2),
            (PF, B, (t, 0, 0), (xd, t, t), 0), (PF, L, (0, 0, t), (t, t, ph - t), 2), (PF, PT, (t, 0, ph - t), (xd, t, ph), 0),
            (PB, B, (t, pd - t, 0), (xd, pd, t), 0), (PB, L, (0, pd - t, t), (t, pd, ph - t), 2), (PB, PT, (t, pd - t, ph - t), (xd, pd, ph), 0)]
        for ji, (A, Bn, lo, hi, ax) in enumerate(joints):
            ln = hi[ax] - lo[ax]; k = round(ln / p['fingerW']); k = max(k, 3); k += (k % 2 == 0)
            seg = ln / k
            before_A = panels[A].Volume(); before_B = panels[Bn].Volume()
            for si in range(k):
                a_ = list(lo); b_ = list(hi); a_[ax] = lo[ax] + si * seg; b_[ax] = lo[ax] + (si + 1) * seg
                if si % 2 == 1: panels[Bn] = panels[Bn].cut(box(a_, b_))
                else: panels[A] = panels[A].cut(box(a_, b_))
            joint_report.append((A, Bn, k, round(seg, 1)))
    cuts = []  # (panel, tool, label)
    for j in range(n):
        sq = (p['bottleD'] + 4) / 2
        cuts.append(('Plinth top', box((bx[j] - sq, yc - sq, ph - t - e), (bx[j] + sq, yc + sq, ph + e)), f'plinthHole{j}'))
    cuts.append(('Plinth top', cyl((p['outX'], yc, ph - t - e), (p['outX'], yc, ph + e), p['outPadD'] / 2), 'outHole'))
    for q in range(n):
        for dx, lab, r in ((0, 'pumpHole', p['pumpHoleD'] / 2), (-p['pumpScrewDx'] / 2, 'screwL', m3), (p['pumpScrewDx'] / 2, 'screwR', m3)):
            cuts.append(('Pump wall', cyl((bx[q] + dx, pwy - e, p['pumpZ']), (bx[q] + dx, pwy + t + e, p['pumpZ']), r), f'{lab}{q}'))
    clipRow = p['tubeOD'] + 3; clipH = n * clipRow + 6; clipD = 16; clipW = 12
    clipZ0 = p['pumpZ'] + p['faceH'] / 2 + p['barbRoom']; clipZs = clipZ0 + clipH / 2
    clipX = [bx[i] + p['bottlePitch'] / 2 for i in range(n)]
    nSlots = p['nStore'] + (1 if p['funnelD'] > 0 else 0); rackSp = p['outD'] + 6; rackD = p['outD'] + 8; rackH = 30
    rackL = nSlots * rackSp + 4; rackY0 = D / 2 - rackL / 2; rz = p['rackZ']
    rackScrewY = [rackY0 + 2 + rackSp, rackY0 + 2 + (nSlots - 1) * rackSp] if nSlots >= 2 else []
    for i, ry in enumerate(rackScrewY):
        cuts.append(('Right side', cyl((W - t - e, ry, rz + rackH/2), (W + e, ry, rz + rackH/2), m3), f'rackScrew{i}'))
    for i, cx in enumerate(clipX):
        cuts.append(('Pump wall', cyl((cx, pwy - e, clipZs), (cx, pwy + t + e, clipZs), m3), f'clipScrew{i}'))
    cuts.append(('Divider wall', box((xd - e, pwy + t + 5, H - 80), (xd + t + e, D - t - 5, H - 40)), 'wireSlot'))
    cuts.append(('Divider wall', cyl((xd - e, yc, t + 12), (xd + t + e, yc, t + 12), 5), 'ledHole'))
    cuts.append(('Back (screw-on)', cyl((xb, D - t - e, 60), (xb, D + e, 60), p['jackD'] / 2), 'jack'))
    cuts.append(('Back (screw-on)', box((xb - 7, D - t - e, 21), (xb + 7, D + e, 29)), 'usb'))
    if p['hasSwitch']:
        cuts.append(('Bay front (QR)', box((xb - p['swW'] / 2, -e, 60 - p['swH'] / 2), (xb + p['swW'] / 2, t + e, 60 + p['swH'] / 2)), 'switch'))
    cuts.append(('Bay front (QR)', box((xb + p['winOff'] - p['winW']/2, -e, p['scrZ'] - p['winH']/2), (xb + p['winOff'] + p['winW']/2, t + e, p['scrZ'] + p['winH']/2)), 'screenWin'))
    for i, (sx_, sz_) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        hx, hz = xb + sx_ * p['scrHoleDx']/2, p['scrZ'] + sz_ * p['scrHoleDz']/2
        cuts.append(('Bay front (QR)', cyl((hx, -e, hz), (hx, t + e, hz), m3), f'screenHole{i}'))
    for s in (-1, 1):
        cuts.append(('Top (wet bay)', cyl((p['outX'] + s * p['nozDx'] / 2, yc, H - t - e), (p['outX'] + s * p['nozDx'] / 2, yc, H + e), m3), f'noz{s}'))
    bs = 12
    backBlocks = [(t, t), (W - t - bs, t), (t, H - t - bs), (W - t - bs, H - t - bs)]
    lidBlocks = [(xd + t, t), (W - t - bs, t), (xd + t, D - t - bs)]
    for i, (x, z) in enumerate(backBlocks):
        cuts.append(('Back (screw-on)', cyl((x + bs/2, D - t - e, z + bs/2), (x + bs/2, D + e, z + bs/2), m3), f'backScrew{i}'))
    for i, (x, y) in enumerate(lidBlocks):
        cuts.append(('Bay lid (screw-on)', cyl((x + bs/2, y + bs/2, H - t - e), (x + bs/2, y + bs/2, H + e), m3), f'lidScrew{i}'))
    for k, (sx, sy) in enumerate(((-1, -1), (1, -1), (-1, 1), (1, 1))):
        cx, cy = xb + sx * p['boardDx'] / 2, D / 2 + sy * p['boardDy'] / 2
        cuts.append(('Base', cyl((cx, cy, -e), (cx, cy, t + e), m3), f'standoff{k}'))

    problems = []
    for pan, tool, lab in cuts:
        before = panels[pan].Volume()
        panels[pan] = panels[pan].cut(tool)
        if not panels[pan].Volume() < before - 1e-6:
            problems.append(f'cut {lab} does not touch {pan}')
        # a cut must not slice through the edge of a panel (would split / notch it)
        if len(panels[pan].Solids()) != 1:
            problems.append(f'cut {lab} split {pan}')

    refs = {}
    for r in range(n):
        hb = p['bottleD'] / 2
        refs[f'REF bottle {r+1}'] = box((bx[r] - hb, yc - hb, ph - p['cupDepth']), (bx[r] + hb, yc + hb, ph - p['cupDepth'] + p['bottleH']))
        refs[f'REF pump head {r+1}'] = box((bx[r] - p['faceW'] / 2, pwy - p['headDepth'], p['pumpZ'] - p['faceH'] / 2), (bx[r] + p['faceW'] / 2, pwy, p['pumpZ'] + p['faceH'] / 2))
        refs[f'REF pump motor {r+1}'] = cyl((bx[r], pwy + t, p['pumpZ']), (bx[r], pwy + t + p['motorL'], p['pumpZ']), p['motorD'] / 2)
    for sb in range(p['nStore']):
        sy_ = rackY0 + 2 + (sb + 0.5) * rackSp
        refs[f'REF stored bottle {sb+1}'] = cyl((W + rackD/2, sy_, rz + 3), (W + rackD/2, sy_, rz + 3 + p['outH'] + 25), p['outD'] / 2)
    refs['REF 2.8in screen'] = box((xb - p['scrW']/2, t + 4, p['scrZ'] - p['scrH']/2), (xb + p['scrW']/2, t + 10, p['scrZ'] + p['scrH']/2))
    refs['REF output bottle'] = cyl((p['outX'], yc, ph + p['padH']), (p['outX'], yc, ph + p['padH'] + p['outH']), p['outD'] / 2)

    # printed parts
    prints = {}
    cupR = (p['bottleD'] + 4) / 2; seat = ph + p['padH']; tipZ = seat + p['outH'] + p['funnelH'] + p['nozGap']
    padRimR = p['outPadD'] / 2 + p['flangeW'] + 3; g = p['tubeOD'] + 1.5; bw = g + p['tubeOD'] + 4; tipH = 12
    plateZ0 = H - t - 4; pilot = 1.4; ox = p['outX']
    for c in range(n):
        zb = ph - p['cupDepth'] - p['cupFloor']
        bo = cupR - p['fitClear']; fo = cupR + p['flangeW']; io = (p['bottleD'] + 1) / 2
        body = box((bx[c] - bo, yc - bo, zb), (bx[c] + bo, yc + bo, ph)).fuse(box((bx[c] - fo, yc - fo, ph), (bx[c] + fo, yc + fo, ph + p['flangeT'])))
        prints[f'PRINT diffuser cup {c+1}'] = body.cut(box((bx[c] - io, yc - io, ph - p['cupDepth']), (bx[c] + io, yc + io, ph + p['flangeT'] + e))).clean()
    holeR = p['outPadD'] / 2 - p['fitClear']
    pad = cyl((ox, yc, ph - t - 3), (ox, yc, seat), holeR).fuse(cyl((ox, yc, ph), (ox, yc, seat + 4), padRimR))
    pad = pad.cut(cyl((ox, yc, seat), (ox, yc, seat + 4 + e), p['outD'] / 2 + 0.5)).cut(cyl((ox, yc, ph - t - 3 - e), (ox, yc, seat - 0.8), holeR - 1.2))
    prints['PRINT output pad'] = pad.clean()
    noz = box((ox - bw/2, yc - bw/2, tipZ), (ox + bw/2, yc + bw/2, tipZ + tipH)).fuse(
        box((ox + bw/2 - 1.5, yc - 6, tipZ), (ox + bw/2 + 4, yc + 6, plateZ0 + 1))).fuse(
        box((ox - p['nozDx']/2 - 6, yc - 8, plateZ0), (ox + p['nozDx']/2 + 6, yc + 8, H - t)))
    guideZ = (tipZ + tipH + plateZ0) / 2
    noz = noz.fuse(box((ox - bw/2, yc - bw/2, guideZ), (ox + bw/2, yc + bw/2, guideZ + 6)))
    for sx_, sy_ in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
        noz = noz.cut(cyl((ox + sx_*g/2, yc + sy_*g/2, tipZ - e), (ox + sx_*g/2, yc + sy_*g/2, guideZ + 6 + e), (p['tubeOD'] + 0.3) / 2))
    for s_ in (-1, 1):
        noz = noz.cut(cyl((ox + s_*p['nozDx']/2, yc, plateZ0 - e), (ox + s_*p['nozDx']/2, yc, H - t + e), pilot))
    prints['PRINT nozzle holder'] = noz.clean()
    rk = box((W, rackY0, rz), (W + rackD, rackY0 + rackL, rz + rackH))
    for so in range(nSlots):
        sy_ = rackY0 + 2 + (so + 0.5) * rackSp
        if p['funnelD'] > 0 and so == nSlots - 1:
            rk = rk.cut(cyl((W + rackD/2, sy_, rz - e), (W + rackD/2, sy_, rz + rackH + e), p['funnelD'] * 0.3))
        else:
            rk = rk.cut(cyl((W + rackD/2, sy_, rz + 3), (W + rackD/2, sy_, rz + rackH + e), p['outD'] / 2 + 0.5))
    for ry in rackScrewY:
        rk = rk.cut(cyl((W - e, ry, rz + rackH/2), (W + 9, ry, rz + rackH/2), pilot))
    prints['PRINT storage rack'] = rk
    for tc, cx in enumerate(clipX):
        blk = box((cx - clipW/2, pwy - clipD, clipZ0), (cx + clipW/2, pwy, clipZ0 + clipH))
        for row in range(n):
            gz = clipZ0 + clipH - 3 - (row + 0.5) * clipRow; gy = pwy - clipD + 0.425 * p['tubeOD']
            blk = blk.cut(cyl((cx - clipW/2 - e, gy, gz), (cx + clipW/2 + e, gy, gz), p['tubeOD'] / 2 + 0.2))
        blk = blk.cut(cyl((cx, pwy - 9, clipZs), (cx, pwy + e, clipZs), pilot))
        prints[f'PRINT tube clip {tc+1}'] = blk
    for i, (x, z) in enumerate(backBlocks):
        prints[f'PRINT screw block back {i+1}'] = box((x, D - t - bs, z), (x + bs, D - t, z + bs)).cut(cyl((x + bs/2, D - t - bs - e, z + bs/2), (x + bs/2, D - t + e, z + bs/2), pilot))
    for i, (x, y) in enumerate(lidBlocks):
        prints[f'PRINT screw block lid {i+1}'] = box((x, y, H - t - bs), (x + bs, y + bs, H - t)).cut(cyl((x + bs/2, y + bs/2, H - t - bs - e), (x + bs/2, y + bs/2, H - t + e), pilot))
    for nm, sh in prints.items():
        if len(sh.Solids()) != 1: problems.append(f'{nm} is {len(sh.Solids())} solids')
    # printed parts must line up with the panel screw holes: the pilot axis must pass through the panel clearance hole
    for nm, sh in prints.items():
        for pn, ps in panels.items():
            v = sh.intersect(ps).Volume()
            if v > 1e-3: problems.append(f'{nm} collides with {pn} ({v:.1f} mm3)')
        for rn, rs in refs.items() if False else []:
            pass
    refs_for_check = None

    # interference: ref parts vs panels, panels vs panels
    for rn, rs in refs.items():
        for pn, ps in panels.items():
            v = rs.intersect(ps).Volume()
            if v > 1e-3:
                problems.append(f'{rn} collides with {pn} ({v:.0f} mm3)')
    names = list(panels)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            v = panels[names[i]].intersect(panels[names[j]]).Volume()
            if v > 1e-3:
                problems.append(f'{names[i]} overlaps {names[j]} ({v:.0f} mm3)')
    for nm, sh in prints.items():
        for rn, rs in refs.items():
            v = sh.intersect(rs).Volume()
            if v > 1e-3: problems.append(f'{nm} collides with {rn} ({v:.1f} mm3)')
    names2 = list(prints)
    for i in range(len(names2)):
        for j in range(i + 1, len(names2)):
            v = prints[names2[i]].intersect(prints[names2[j]]).Volume()
            if v > 1e-3: problems.append(f'{names2[i]} overlaps {names2[j]}')
    # light path: diffuser floor and pad floor thickness, nozzle clearance
    info = dict(cup_bottom_z=ph - p['cupDepth'] - p['cupFloor'], tipZ=tipZ, bottle_top=seat + p['outH'],
                liquidZ=ph - p['cupDepth'] + p['bottleH'] - 20, bundle_w=g + p['tubeOD'], arm_len=plateZ0 - tipZ - tipH)
    info['joints'] = joint_report
    info['clip_top'] = clipZ0 + clipH
    info['bottle_top'] = ph - p['cupDepth'] + p['bottleH']; info['pump_bottom'] = p['pumpZ'] - p['faceH']/2
    info['tipZ'] = tipZ; info['funnel_top'] = seat + p['outH'] + p['funnelH']; info['rackL'] = rackL
    return panels, refs, prints, problems, D, info


if __name__ == '__main__':
    panels, refs, prints, problems, D, info = build(P)
    print('Depth D =', D, info)
    print('\n'.join(problems) or 'no problems')
    asm = cq.Assembly()
    for nme, s in panels.items():
        asm.add(s, name=nme.replace(' ', '_').replace('(', '').replace(')', ''), color=cq.Color(0.82, 0.68, 0.47))
    for nme, s in prints.items():
        asm.add(s, name=nme.replace(' ', '_'), color=cq.Color(0.95, 0.95, 0.95))
    for nme, s in refs.items():
        asm.add(s, name=nme.replace(' ', '_'), color=cq.Color(0.4, 0.6, 0.9) if 'bottle' in nme else cq.Color(0.3, 0.7, 0.55))
    asm.save('scentty_chassis_preview.step')
    import pickle
    for nme, s in {**panels, **refs, **prints}.items():
        cq.exporters.export(cq.Workplane().add(s), f'stl/{nme}.stl', tolerance=0.2, angularTolerance=0.2)
