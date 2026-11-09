"""Tiny z-buffer renderer for the STL preview (orthographic, flat shading, outlines)."""
import glob, os, sys
import numpy as np
import trimesh
from PIL import Image, ImageDraw, ImageFont

HIDE = {'Right side', 'Top (wet bay)', 'Bay lid (screw-on)'}
WOOD = np.array([214, 184, 140]); BOTTLE = np.array([240, 160, 70]); OUT = np.array([150, 120, 220]); PUMP = np.array([70, 165, 140])


def color_for(name):
    if name.startswith('REF bottle'): return BOTTLE
    if name.startswith('REF output'): return OUT
    if name.startswith('REF'): return PUMP
    return WOOD


def render(cam, path, size=(1400, 1000), title=''):
    cam = np.asarray(cam, float); cam /= np.linalg.norm(cam)
    up = np.array([0, 0, 1.0]); right = np.cross(up, cam); right /= np.linalg.norm(right); up2 = np.cross(cam, right)
    light = cam * 0.6 + np.array([0.2, -0.3, 0.8]); light /= np.linalg.norm(light)
    meshes = []
    for f in sorted(glob.glob('stl/*.stl')):
        name = os.path.basename(f)[:-4]
        if name in HIDE: continue
        meshes.append((name, trimesh.load(f)))
    allv = np.vstack([m.vertices for _, m in meshes])
    P = lambda v: np.c_[v @ right, v @ up2, v @ cam]
    pv = P(allv); mn, mx = pv[:, :2].min(0), pv[:, :2].max(0)
    Wd, Hd = size; s = 0.9 * min(Wd / (mx[0] - mn[0]), Hd / (mx[1] - mn[1]))
    off = np.array([Wd, Hd]) / 2 - s * (mn + mx) / 2
    zbuf = np.full((Hd, Wd), -np.inf); img = np.full((Hd, Wd, 3), 255.0); idb = np.full((Hd, Wd), -1)
    oid = 0
    for name, m in meshes:
        base = color_for(name)
        tri = P(m.triangles.reshape(-1, 3)).reshape(-1, 3, 3)
        xy = tri[:, :, :2] * s + off; xy[:, :, 1] = Hd - xy[:, :, 1]
        shade = 0.5 + 0.5 * np.clip(m.face_normals @ light, 0, 1)
        for k in range(len(tri)):
            (x0, y0), (x1, y1), (x2, y2) = xy[k]
            z = tri[k, :, 2]
            bx0, bx1 = int(max(0, np.floor(min(x0, x1, x2)))), int(min(Wd - 1, np.ceil(max(x0, x1, x2))))
            by0, by1 = int(max(0, np.floor(min(y0, y1, y2)))), int(min(Hd - 1, np.ceil(max(y0, y1, y2))))
            if bx1 < bx0 or by1 < by0: continue
            den = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2)
            if abs(den) < 1e-9: continue
            X, Y = np.meshgrid(np.arange(bx0, bx1 + 1) + 0.5, np.arange(by0, by1 + 1) + 0.5)
            a = ((y1 - y2) * (X - x2) + (x2 - x1) * (Y - y2)) / den
            b = ((y2 - y0) * (X - x2) + (x0 - x2) * (Y - y2)) / den
            c = 1 - a - b
            inside = (a >= -1e-6) & (b >= -1e-6) & (c >= -1e-6)
            if not inside.any(): continue
            zz = a * z[0] + b * z[1] + c * z[2]
            sub = zbuf[by0:by1 + 1, bx0:bx1 + 1]
            upd = inside & (zz > sub)
            sub[upd] = zz[upd]
            img[by0:by1 + 1, bx0:bx1 + 1][upd] = base * shade[k]
            idb[by0:by1 + 1, bx0:bx1 + 1][upd] = oid * 100000 + int(round(shade[k] * 50))
        oid += 1
    # outlines where object id / facet shade changes
    e = np.zeros_like(idb, bool)
    e[:, 1:] |= idb[:, 1:] != idb[:, :-1]; e[1:, :] |= idb[1:, :] != idb[:-1, :]
    img[e] = img[e] * 0.45
    im = Image.fromarray(img.clip(0, 255).astype(np.uint8))
    if title:
        d = ImageDraw.Draw(im)
        try: font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 26)
        except Exception: font = ImageFont.load_default()
        d.text((30, 25), title, fill=(40, 40, 40), font=font)
    im.save(path)


if __name__ == '__main__':
    render((0.55, -1.0, 0.65), 'scentty_chassis_preview.png',
           title='ScenTTy chassis, default values (right side, top and bay lid hidden)')
