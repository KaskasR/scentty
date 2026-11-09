// =====================================================================
//  ScenTTy chassis — parametric Onshape custom feature
//
//  HOW TO USE
//  1. In your Onshape document: + (bottom-left) > Create Feature Studio.
//  2. KEEP the two lines Onshape puts at the top
//     ("FeatureScript ####;" and the "import(...)" line).
//     Paste EVERYTHING in this file underneath them. Click Commit.
//  3. Open a Part Studio > "Add custom features" (puzzle-piece icon on the
//     right of the toolbar) > pick this Feature Studio > "ScenTTy chassis".
//  4. Click the new toolbar button. Every dimension is a field in the dialog.
//     To drive a field from a Variable, type #name (e.g. #bottle_d).
//
//  Axes: X = width (left->right seen from the front), Y = depth (front = 0),
//        Z = height (base = 0). Front of the machine faces -Y (Front view).
//  Parts named "REF ..." are placeholders for checking fit. Parts named
//  "PRINT ..." are 3D-printed (right-click > Export as STL). Turn both
//  toggles off before exporting panels for the laser.
// =====================================================================

const T_B         = { (millimeter) : [1, 3, 20] } as LengthBoundSpec;
const W_B         = { (millimeter) : [100, 560, 2000] } as LengthBoundSpec;
const H_B         = { (millimeter) : [100, 300, 2000] } as LengthBoundSpec;
const EBAY_B      = { (millimeter) : [40, 120, 1000] } as LengthBoundSpec;
const PLINTH_H_B  = { (millimeter) : [10, 35, 500] } as LengthBoundSpec;
const PLINTH_D_B  = { (millimeter) : [20, 110, 1000] } as LengthBoundSpec;
const CUP_B       = { (millimeter) : [0, 8, 100] } as LengthBoundSpec;
const BOTTLES_B   = { (unitless) : [1, 4, 8] } as IntegerBoundSpec;
const BOTTLE_D_B  = { (millimeter) : [10, 38.1, 300] } as LengthBoundSpec;
const BOTTLE_H_B  = { (millimeter) : [10, 140, 500] } as LengthBoundSpec;
const PITCH_B     = { (millimeter) : [10, 75, 500] } as LengthBoundSpec;
const BX0_B       = { (millimeter) : [10, 55, 1000] } as LengthBoundSpec;
const OUT_X_B     = { (millimeter) : [10, 375, 2000] } as LengthBoundSpec;
const OUT_PAD_B   = { (millimeter) : [10, 45, 300] } as LengthBoundSpec;
const OUT_D_B     = { (millimeter) : [10, 30, 300] } as LengthBoundSpec;
const OUT_H_B     = { (millimeter) : [10, 80, 500] } as LengthBoundSpec;
const PAD_H_B     = { (millimeter) : [0, 10, 300] } as LengthBoundSpec;
const PWY_B       = { (millimeter) : [20, 150, 1000] } as LengthBoundSpec;
const CHASE_B     = { (millimeter) : [10, 60, 500] } as LengthBoundSpec;
const PUMP_Z_B    = { (millimeter) : [20, 200, 2000] } as LengthBoundSpec;
const PUMP_HOLE_B = { (millimeter) : [3, 30, 200] } as LengthBoundSpec;
const SCREW_DX_B  = { (millimeter) : [5, 48.5, 300] } as LengthBoundSpec;
const FACE_W_B    = { (millimeter) : [5, 54.5, 300] } as LengthBoundSpec;
const FACE_H_B    = { (millimeter) : [5, 40.3, 300] } as LengthBoundSpec;
const HEAD_B      = { (millimeter) : [5, 26.5, 300] } as LengthBoundSpec;
const MOTOR_L_B   = { (millimeter) : [5, 44, 300] } as LengthBoundSpec;
const MOTOR_D_B   = { (millimeter) : [5, 29, 200] } as LengthBoundSpec;
const JACK_B      = { (millimeter) : [3, 8, 50] } as LengthBoundSpec;
const SW_W_B      = { (millimeter) : [3, 19, 100] } as LengthBoundSpec;
const SW_H_B      = { (millimeter) : [3, 13, 100] } as LengthBoundSpec;
const BOARD_DX_B  = { (millimeter) : [5, 62, 500] } as LengthBoundSpec;
const BOARD_DY_B  = { (millimeter) : [5, 82, 500] } as LengthBoundSpec;
const NOZ_DX_B    = { (millimeter) : [5, 30, 300] } as LengthBoundSpec;
const BED_W_B     = { (millimeter) : [100, 610, 3000] } as LengthBoundSpec;
const BED_H_B     = { (millimeter) : [100, 305, 3000] } as LengthBoundSpec;
const CLEAR_B     = { (millimeter) : [0, 0.2, 2] } as LengthBoundSpec;
const FLANGE_W_B  = { (millimeter) : [1, 4, 30] } as LengthBoundSpec;
const FLANGE_T_B  = { (millimeter) : [0.6, 2, 10] } as LengthBoundSpec;
const FLOOR_B     = { (millimeter) : [0.2, 0.6, 5] } as LengthBoundSpec;
const TUBE_OD_B   = { (millimeter) : [1, 5, 20] } as LengthBoundSpec;
const NECK_B      = { (millimeter) : [3, 12, 100] } as LengthBoundSpec;
const NOZ_GAP_B   = { (millimeter) : [-30, 15, 80] } as LengthBoundSpec;
const FINGER_B    = { (millimeter) : [5, 20, 200] } as LengthBoundSpec;
const FUNNEL_D_B  = { (millimeter) : [0, 28, 200] } as LengthBoundSpec;
const FUNNEL_H_B  = { (millimeter) : [0, 20, 200] } as LengthBoundSpec;
const BARB_B      = { (millimeter) : [0, 20, 100] } as LengthBoundSpec;
const RACK_Z_B    = { (millimeter) : [0, 100, 2000] } as LengthBoundSpec;
const STORE_B     = { (unitless) : [0, 4, 8] } as IntegerBoundSpec;
const SCR_W_B     = { (millimeter) : [10, 86, 300] } as LengthBoundSpec;
const SCR_H_B     = { (millimeter) : [10, 50, 300] } as LengthBoundSpec;
const WIN_W_B     = { (millimeter) : [5, 58.6, 300] } as LengthBoundSpec;
const WIN_H_B     = { (millimeter) : [5, 44.2, 300] } as LengthBoundSpec;
const WIN_OFF_B   = { (millimeter) : [-50, 0, 50] } as LengthBoundSpec;
const SCR_HDX_B   = { (millimeter) : [5, 76.08, 300] } as LengthBoundSpec;
const SCR_HDZ_B   = { (millimeter) : [5, 44, 300] } as LengthBoundSpec;
const SCR_Z_B     = { (millimeter) : [20, 230, 2000] } as LengthBoundSpec;

annotation { "Feature Type Name" : "ScenTTy chassis",
             "Feature Type Description" : "Laser-cut plywood chassis for the ScenTTy perfume maker" }
export const scenttyChassis = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        // ---------- Material and box ----------
        annotation { "Name" : "Sheet thickness (t)" }
        isLength(definition.t, T_B);
        annotation { "Name" : "Overall width (W)" }
        isLength(definition.W, W_B);
        annotation { "Name" : "Overall height (H)" }
        isLength(definition.H, H_B);
        annotation { "Name" : "Electronics bay width" }
        isLength(definition.ebayW, EBAY_B);

        // ---------- Plinth and bottles ----------
        annotation { "Name" : "Plinth height" }
        isLength(definition.plinthH, PLINTH_H_B);
        annotation { "Name" : "Plinth depth" }
        isLength(definition.plinthD, PLINTH_D_B);
        annotation { "Name" : "Diffuser cup depth below plinth top" }
        isLength(definition.cupDepth, CUP_B);
        annotation { "Name" : "Number of reservoir bottles" }
        isInteger(definition.nBottles, BOTTLES_B);
        annotation { "Name" : "Reservoir bottle width (square side)" }
        isLength(definition.bottleD, BOTTLE_D_B);
        annotation { "Name" : "Reservoir bottle height (with cap)" }
        isLength(definition.bottleH, BOTTLE_H_B);
        annotation { "Name" : "Bottle pitch (center to center)" }
        isLength(definition.bottlePitch, PITCH_B);
        annotation { "Name" : "First bottle X (from left edge)" }
        isLength(definition.bottleX0, BX0_B);

        // ---------- Output station ----------
        annotation { "Name" : "Output bottle X (from left edge)" }
        isLength(definition.outX, OUT_X_B);
        annotation { "Name" : "Output pad hole diameter" }
        isLength(definition.outPadD, OUT_PAD_B);
        annotation { "Name" : "Output bottle diameter" }
        isLength(definition.outD, OUT_D_B);
        annotation { "Name" : "Output bottle height" }
        isLength(definition.outH, OUT_H_B);
        annotation { "Name" : "Output pad raise above plinth" }
        isLength(definition.padH, PAD_H_B);
        annotation { "Name" : "Nozzle holder screw spacing" }
        isLength(definition.nozDx, NOZ_DX_B);

        // ---------- Pumps ----------
        annotation { "Name" : "Pump wall Y (front face)" }
        isLength(definition.pumpWallY, PWY_B);
        annotation { "Name" : "Chase depth behind pump wall" }
        isLength(definition.chaseD, CHASE_B);
        annotation { "Name" : "Pump center height (Z)" }
        isLength(definition.pumpZ, PUMP_Z_B);
        annotation { "Name" : "Pump motor through-hole diameter" }
        isLength(definition.pumpHoleD, PUMP_HOLE_B);
        annotation { "Name" : "Pump faceplate screw spacing" }
        isLength(definition.pumpScrewDx, SCREW_DX_B);
        annotation { "Name" : "Pump faceplate width" }
        isLength(definition.faceW, FACE_W_B);
        annotation { "Name" : "Pump faceplate height" }
        isLength(definition.faceH, FACE_H_B);
        annotation { "Name" : "Pump head depth (in front of wall)" }
        isLength(definition.headDepth, HEAD_B);
        annotation { "Name" : "Motor length (behind wall)" }
        isLength(definition.motorL, MOTOR_L_B);
        annotation { "Name" : "Motor diameter" }
        isLength(definition.motorD, MOTOR_D_B);
        annotation { "Name" : "Room above pump head for barbs + tube bend" }
        isLength(definition.barbRoom, BARB_B);

        // ---------- Electronics ----------
        annotation { "Name" : "DC jack hole diameter" }
        isLength(definition.jackD, JACK_B);
        annotation { "Name" : "Power switch cutout", "Default" : false }
        definition.hasSwitch is boolean;
        annotation { "Name" : "Power switch cutout width" }
        isLength(definition.swW, SW_W_B);
        annotation { "Name" : "Power switch cutout height" }
        isLength(definition.swH, SW_H_B);
        annotation { "Name" : "Screen center height (Z)" }
        isLength(definition.scrZ, SCR_Z_B);
        annotation { "Name" : "Screen board width" }
        isLength(definition.scrW, SCR_W_B);
        annotation { "Name" : "Screen board height" }
        isLength(definition.scrH, SCR_H_B);
        annotation { "Name" : "Screen window width" }
        isLength(definition.winW, WIN_W_B);
        annotation { "Name" : "Screen window height" }
        isLength(definition.winH, WIN_H_B);
        annotation { "Name" : "Screen window offset (X, from board center)" }
        isLength(definition.winOff, WIN_OFF_B);
        annotation { "Name" : "Screen mounting hole spacing X" }
        isLength(definition.scrHoleDx, SCR_HDX_B);
        annotation { "Name" : "Screen mounting hole spacing Z" }
        isLength(definition.scrHoleDz, SCR_HDZ_B);
        annotation { "Name" : "Board standoff spacing X" }
        isLength(definition.boardDx, BOARD_DX_B);
        annotation { "Name" : "Board standoff spacing Y" }
        isLength(definition.boardDy, BOARD_DY_B);

        // ---------- Finger joints ----------
        annotation { "Name" : "Finger joints", "Default" : true }
        definition.joints is boolean;
        annotation { "Name" : "Finger width (target)" }
        isLength(definition.fingerW, FINGER_B);

        // ---------- Checks and display ----------
        annotation { "Name" : "Laser bed width" }
        isLength(definition.bedW, BED_W_B);
        annotation { "Name" : "Laser bed height" }
        isLength(definition.bedH, BED_H_B);
        annotation { "Name" : "Show reference parts (bottles, pumps)", "Default" : true }
        definition.showRef is boolean;

        // ---------- 3D-printed parts ----------
        annotation { "Name" : "Show printed parts", "Default" : true }
        definition.showPrinted is boolean;
        annotation { "Name" : "Print fit clearance (per side)" }
        isLength(definition.fitClear, CLEAR_B);
        annotation { "Name" : "Flange overhang" }
        isLength(definition.flangeW, FLANGE_W_B);
        annotation { "Name" : "Flange thickness" }
        isLength(definition.flangeT, FLANGE_T_B);
        annotation { "Name" : "Diffuser floor thickness" }
        isLength(definition.cupFloor, FLOOR_B);
        annotation { "Name" : "Tubing outer diameter" }
        isLength(definition.tubeOD, TUBE_OD_B);
        annotation { "Name" : "Output bottle neck inner diameter" }
        isLength(definition.neckID, NECK_B);
        annotation { "Name" : "Funnel top diameter (0 = no funnel)" }
        isLength(definition.funnelD, FUNNEL_D_B);
        annotation { "Name" : "Funnel height above bottle top" }
        isLength(definition.funnelH, FUNNEL_H_B);
        annotation { "Name" : "Stored output bottles (rack on right side)" }
        isInteger(definition.nStore, STORE_B);
        annotation { "Name" : "Storage rack height (Z)" }
        isLength(definition.rackZ, RACK_Z_B);
        annotation { "Name" : "Nozzle gap above output bottle" }
        isLength(definition.nozGap, NOZ_GAP_B);
    }
    {
        const t = definition.t;
        const W = definition.W;
        const H = definition.H;
        const ph = definition.plinthH;
        const pd = definition.plinthD;
        const pwy = definition.pumpWallY;
        const xd = W - definition.ebayW;               // divider left face
        const D = pwy + t + definition.chaseD + t;      // overall depth (derived)
        const yc = pd / 2;                              // bottle row, depth center
        const xb = (xd + W) / 2;                        // electronics bay center X
        const e = 1 * millimeter;                       // overshoot so cuts go fully through
        const m3 = 1.6 * millimeter;                    // M3 clearance radius (3.2 mm hole)
        const n = definition.nBottles;

        var bx = [];
        for (var i = 0; i < n; i += 1)
        {
            bx = append(bx, definition.bottleX0 + i * definition.bottlePitch);
        }

        // Tube clips: one between each pump and the next (last one right of the last pump), above the pumps.
        // Each clip has one snap-in groove per pump, stacked: pump 1 on top, last pump at the bottom,
        // so every outlet tube rises only to its own groove and never crosses another tube.
        const clipRow = definition.tubeOD + 3 * millimeter;        // groove spacing
        const clipH = n * clipRow + 6 * millimeter;                // clip height
        const clipD = 16 * millimeter;                             // clip depth off the wall
        const clipW = 12 * millimeter;                             // clip width along the tubes
        const clipZ0 = definition.pumpZ + definition.faceH / 2 + definition.barbRoom;
        const clipZs = clipZ0 + clipH / 2;                         // screw height
        var clipX = [];
        for (var ii = 0; ii < n; ii += 1)
        {
            clipX = append(clipX, bx[ii] + definition.bottlePitch / 2);
        }

        // Storage rack: printed bar on the OUTSIDE of the right panel. One socket per stored output bottle,
        // plus a socket at the back end that holds the funnel point-down.
        const hasFunnel = definition.funnelD > 0 * millimeter;
        const nSlots = definition.nStore + (hasFunnel ? 1 : 0);
        const rackSp = definition.outD + 6 * millimeter;           // socket pitch
        const rackD = definition.outD + 8 * millimeter;            // how far the rack sticks out
        const rackH = 30 * millimeter;
        const rackL = nSlots * rackSp + 4 * millimeter;
        const rackY0 = D / 2 - rackL / 2;
        const rackZ = definition.rackZ;
        var rackScrewY = [];
        if (nSlots >= 2)
        {
            rackScrewY = [rackY0 + 2 * millimeter + rackSp, rackY0 + 2 * millimeter + (nSlots - 1) * rackSp];
        }

        // ------------------------- Panels -------------------------
        // With finger joints on, each "tab" panel is extended by t into the panel it meets;
        // the joints below then split that overlap into alternating tabs and slots.
        const z0mm = 0 * millimeter;
        const J = definition.joints ? t : z0mm;
        const base = makePanel(context, id, "base", "Base",
                vector(z0mm, z0mm, z0mm), vector(W, D, t));
        const left = makePanel(context, id, "left", "Left side",
                vector(z0mm, z0mm, t - J), vector(t, D, H - t + J));
        const right = makePanel(context, id, "right", "Right side",
                vector(W - t, z0mm, t - J), vector(W, D, H - t + J));
        const back = makePanel(context, id, "back", "Back (screw-on)",
                vector(t - J, D - t, t - J), vector(W - t + J, D, H - t + J));
        const topPanel = makePanel(context, id, "top", "Top (wet bay)",
                vector(z0mm, z0mm, H - t), vector(xd + t, D, H));
        const lid = makePanel(context, id, "lid", "Bay lid (screw-on)",
                vector(xd + t, z0mm, H - t), vector(W, D, H));
        const divider = makePanel(context, id, "divider", "Divider wall",
                vector(xd, z0mm, t - J), vector(xd + t, D - t + J, H - t + J));
        const bayFront = makePanel(context, id, "bayFront", "Bay front (QR)",
                vector(xd + t - J, z0mm, t - J), vector(W - t + J, t, H - t + J));
        const pumpWall = makePanel(context, id, "pumpWall", "Pump wall",
                vector(t - J, pwy, t - J), vector(xd, pwy + t, H - t + J));
        const plinthTop = makePanel(context, id, "plinthTop", "Plinth top (lift-off)",
                vector(t, z0mm, ph - t), vector(xd, pd, ph));
        const plinthFront = makePanel(context, id, "plinthFront", "Plinth front",
                vector(t - J, z0mm, t - J), vector(xd, t, ph - t + J));
        const plinthBack = makePanel(context, id, "plinthBack", "Plinth back",
                vector(t - J, pd - t, t - J), vector(xd, pd, ph - t + J));

        // ------------------------- Finger joints -------------------------
        if (definition.joints)
        {
            // Remove the corner cubes the extensions created where two extended edges meet
            const corners = [
                [back, vector(z0mm, D - t, z0mm), vector(t, D, t)],
                [back, vector(W - t, D - t, z0mm), vector(W, D, t)],
                [back, vector(z0mm, D - t, H - t), vector(t, D, H)],
                [back, vector(W - t, D - t, H - t), vector(W, D, H)],
                [divider, vector(xd, D - t, z0mm), vector(xd + t, D, t)],
                [divider, vector(xd, D - t, H - t), vector(xd + t, D, H)],
                [bayFront, vector(xd, z0mm, z0mm), vector(xd + t, t, t)],
                [bayFront, vector(W - t, z0mm, z0mm), vector(W, t, t)],
                [bayFront, vector(xd, z0mm, H - t), vector(xd + t, t, H)],
                [bayFront, vector(W - t, z0mm, H - t), vector(W, t, H)],
                [pumpWall, vector(z0mm, pwy, z0mm), vector(t, pwy + t, t)],
                [pumpWall, vector(z0mm, pwy, H - t), vector(t, pwy + t, H)],
                [plinthFront, vector(z0mm, z0mm, z0mm), vector(t, t, t)],
                [plinthFront, vector(z0mm, z0mm, ph - t), vector(t, t, ph)],
                [plinthBack, vector(z0mm, pd - t, z0mm), vector(t, pd, t)],
                [plinthBack, vector(z0mm, pd - t, ph - t), vector(t, pd, ph)]
            ];
            for (var ci = 0; ci < size(corners); ci += 1)
            {
                cutBox(context, id, "corner", ci, corners[ci][0], corners[ci][1], corners[ci][2]);
            }

            // [tab panel, slot panel, joint region lo, hi, length axis (0 = X, 1 = Y, 2 = Z)]
            // Not jointed (glue them): pump wall, plinth front/back to the divider.
            // Plinth top only receives tabs from below, so it lifts off for LED access.
            const fw = definition.fingerW;
            const joints = [
                [left, base, vector(z0mm, z0mm, z0mm), vector(t, D, t), 1],
                [left, topPanel, vector(z0mm, z0mm, H - t), vector(t, D, H), 1],
                [right, base, vector(W - t, z0mm, z0mm), vector(W, D, t), 1],
                [right, lid, vector(W - t, z0mm, H - t), vector(W, D, H), 1],
                [back, base, vector(t, D - t, z0mm), vector(W - t, D, t), 0],
                [back, left, vector(z0mm, D - t, t), vector(t, D, H - t), 2],
                [back, right, vector(W - t, D - t, t), vector(W, D, H - t), 2],
                [back, topPanel, vector(t, D - t, H - t), vector(xd + t, D, H), 0],
                [back, lid, vector(xd + t, D - t, H - t), vector(W - t, D, H), 0],
                [divider, base, vector(xd, z0mm, z0mm), vector(xd + t, D - t, t), 1],
                [divider, topPanel, vector(xd, z0mm, H - t), vector(xd + t, D - t, H), 1],
                [divider, back, vector(xd, D - t, t), vector(xd + t, D, H - t), 2],
                [bayFront, base, vector(xd + t, z0mm, z0mm), vector(W - t, t, t), 0],
                [bayFront, lid, vector(xd + t, z0mm, H - t), vector(W - t, t, H), 0],
                [bayFront, divider, vector(xd, z0mm, t), vector(xd + t, t, H - t), 2],
                [bayFront, right, vector(W - t, z0mm, t), vector(W, t, H - t), 2],
                [pumpWall, base, vector(t, pwy, z0mm), vector(xd, pwy + t, t), 0],
                [pumpWall, topPanel, vector(t, pwy, H - t), vector(xd, pwy + t, H), 0],
                [pumpWall, left, vector(z0mm, pwy, t), vector(t, pwy + t, H - t), 2],
                [plinthFront, base, vector(t, z0mm, z0mm), vector(xd, t, t), 0],
                [plinthFront, left, vector(z0mm, z0mm, t), vector(t, t, ph - t), 2],
                [plinthFront, plinthTop, vector(t, z0mm, ph - t), vector(xd, t, ph), 0],
                [plinthBack, base, vector(t, pd - t, z0mm), vector(xd, pd, t), 0],
                [plinthBack, left, vector(z0mm, pd - t, t), vector(t, pd, ph - t), 2],
                [plinthBack, plinthTop, vector(t, pd - t, ph - t), vector(xd, pd, ph), 0]
            ];
            for (var ji = 0; ji < size(joints); ji += 1)
            {
                fingerJoint(context, id, ji, joints[ji][0], joints[ji][1], joints[ji][2], joints[ji][3], joints[ji][4], fw);
            }
        }

        // ------------------------- Cutouts -------------------------
        // Plinth top: diffuser cup holes + output pad hole
        // (square bottles -> square holes)
        const sqh = (definition.bottleD + 4 * millimeter) / 2;
        for (var j = 0; j < n; j += 1)
        {
            cutBox(context, id, "plinthHole", j, plinthTop,
                    vector(bx[j] - sqh, yc - sqh, ph - t - e), vector(bx[j] + sqh, yc + sqh, ph + e));
        }
        cutCyl(context, id, "outHole", 0, plinthTop,
                vector(definition.outX, yc, ph - t - e), vector(definition.outX, yc, ph + e),
                definition.outPadD / 2);

        // Pump wall: motor hole + 2 faceplate screw holes per pump
        for (var q = 0; q < n; q += 1)
        {
            cutCyl(context, id, "pumpHole", q, pumpWall,
                    vector(bx[q], pwy - e, definition.pumpZ), vector(bx[q], pwy + t + e, definition.pumpZ),
                    definition.pumpHoleD / 2);
            cutCyl(context, id, "pumpScrewL", q, pumpWall,
                    vector(bx[q] - definition.pumpScrewDx / 2, pwy - e, definition.pumpZ),
                    vector(bx[q] - definition.pumpScrewDx / 2, pwy + t + e, definition.pumpZ), m3);
            cutCyl(context, id, "pumpScrewR", q, pumpWall,
                    vector(bx[q] + definition.pumpScrewDx / 2, pwy - e, definition.pumpZ),
                    vector(bx[q] + definition.pumpScrewDx / 2, pwy + t + e, definition.pumpZ), m3);
        }

        // Pump wall: one M3 clearance hole per tube clip (screw goes in from the chase side)
        for (var cc = 0; cc < n; cc += 1)
        {
            cutCyl(context, id, "clipScrew", cc, pumpWall,
                    vector(clipX[cc], pwy - e, clipZs), vector(clipX[cc], pwy + t + e, clipZs), m3);
        }

        // Right side: M3 clearance holes for the storage rack screws (heads inside the electronics bay)
        for (var rs = 0; rs < size(rackScrewY); rs += 1)
        {
            cutCyl(context, id, "rackScrew", rs, right,
                    vector(W - t - e, rackScrewY[rs], rackZ + rackH / 2), vector(W + e, rackScrewY[rs], rackZ + rackH / 2), m3);
        }

        // Divider: high wire slot into the chase, low hole for LED wires inside the plinth
        cutBox(context, id, "wireSlot", 0, divider,
                vector(xd - e, pwy + t + 5 * millimeter, H - 80 * millimeter),
                vector(xd + t + e, D - t - 5 * millimeter, H - 40 * millimeter));
        cutCyl(context, id, "ledHole", 0, divider,
                vector(xd - e, yc, t + 12 * millimeter), vector(xd + t + e, yc, t + 12 * millimeter),
                5 * millimeter);

        // Back: DC jack hole + battery USB slot (both in the bay region)
        cutCyl(context, id, "jackHole", 0, back,
                vector(xb, D - t - e, 60 * millimeter), vector(xb, D + e, 60 * millimeter),
                definition.jackD / 2);
        cutBox(context, id, "usbSlot", 0, back,
                vector(xb - 7 * millimeter, D - t - e, 21 * millimeter),
                vector(xb + 7 * millimeter, D + e, 29 * millimeter));

        // Bay front: power switch cutout (QR code gets engraved above it)
        if (definition.hasSwitch)
        {
            cutBox(context, id, "switch", 0, bayFront,
                vector(xb - definition.swW / 2, -e, 60 * millimeter - definition.swH / 2),
                vector(xb + definition.swW / 2, t + e, 60 * millimeter + definition.swH / 2));
        }

        // Bay front: 2.8" TFT window + 4 mounting holes (screen mounts behind the panel on 4 mm spacers)
        cutBox(context, id, "screenWin", 0, bayFront,
                vector(xb + definition.winOff - definition.winW / 2, -e, definition.scrZ - definition.winH / 2),
                vector(xb + definition.winOff + definition.winW / 2, t + e, definition.scrZ + definition.winH / 2));
        const shx = [-1, 1, -1, 1];
        const shz = [-1, -1, 1, 1];
        for (var sh = 0; sh < 4; sh += 1)
        {
            const hx = xb + shx[sh] * definition.scrHoleDx / 2;
            const hz = definition.scrZ + shz[sh] * definition.scrHoleDz / 2;
            cutCyl(context, id, "screenHole", sh, bayFront, vector(hx, -e, hz), vector(hx, t + e, hz), m3);
        }

        // Top: nozzle holder screw holes above the output pad
        cutCyl(context, id, "nozScrewL", 0, topPanel,
                vector(definition.outX - definition.nozDx / 2, yc, H - t - e),
                vector(definition.outX - definition.nozDx / 2, yc, H + e), m3);
        cutCyl(context, id, "nozScrewR", 0, topPanel,
                vector(definition.outX + definition.nozDx / 2, yc, H - t - e),
                vector(definition.outX + definition.nozDx / 2, yc, H + e), m3);

        // Screw blocks: the back panel and bay lid screw into small printed blocks glued in the corners
        const bs = 12 * millimeter;                    // screw block size
        const backBlocks = [[t, t], [W - t - bs, t], [t, H - t - bs], [W - t - bs, H - t - bs]];   // [x, z] lower corners
        const lidBlocks = [[xd + t, t], [W - t - bs, t], [xd + t, D - t - bs]];                     // [x, y] lower corners
        for (var bb = 0; bb < 4; bb += 1)
        {
            const hx = backBlocks[bb][0] + bs / 2;
            const hz = backBlocks[bb][1] + bs / 2;
            cutCyl(context, id, "backScrew", bb, back, vector(hx, D - t - e, hz), vector(hx, D + e, hz), m3);
        }
        for (var lb = 0; lb < 3; lb += 1)
        {
            const hx = lidBlocks[lb][0] + bs / 2;
            const hy = lidBlocks[lb][1] + bs / 2;
            cutCyl(context, id, "lidScrew", lb, lid, vector(hx, hy, H - t - e), vector(hx, hy, H + e), m3);
        }

        // Base: 4 standoff holes for the ESP32 / MOSFET board, centered in the bay
        const sx = [-1, 1, -1, 1];
        const sy = [-1, -1, 1, 1];
        for (var k = 0; k < 4; k += 1)
        {
            const cx = xb + sx[k] * definition.boardDx / 2;
            const cy = D / 2 + sy[k] * definition.boardDy / 2;
            cutCyl(context, id, "standoff", k, base,
                    vector(cx, cy, -e), vector(cx, cy, t + e), m3);
        }

        // ------------------------- Reference parts -------------------------
        if (definition.showRef)
        {
            for (var r = 0; r < n; r += 1)
            {
                refBox(context, id, "refBottle", r, "REF bottle " ~ (r + 1),
                        vector(bx[r] - definition.bottleD / 2, yc - definition.bottleD / 2, ph - definition.cupDepth),
                        vector(bx[r] + definition.bottleD / 2, yc + definition.bottleD / 2, ph - definition.cupDepth + definition.bottleH));
                refBox(context, id, "refPumpHead", r, "REF pump head " ~ (r + 1),
                        vector(bx[r] - definition.faceW / 2, pwy - definition.headDepth, definition.pumpZ - definition.faceH / 2),
                        vector(bx[r] + definition.faceW / 2, pwy, definition.pumpZ + definition.faceH / 2));
                refCyl(context, id, "refMotor", r, "REF pump motor " ~ (r + 1),
                        vector(bx[r], pwy + t, definition.pumpZ),
                        vector(bx[r], pwy + t + definition.motorL, definition.pumpZ),
                        definition.motorD / 2);
            }
            refBox(context, id, "refScreen", 0, "REF 2.8in screen",
                    vector(xb - definition.scrW / 2, t + 4 * millimeter, definition.scrZ - definition.scrH / 2),
                    vector(xb + definition.scrW / 2, t + 10 * millimeter, definition.scrZ + definition.scrH / 2));
            for (var sb = 0; sb < definition.nStore; sb += 1)
            {
                const slotY = rackY0 + 2 * millimeter + (sb + 0.5) * rackSp;
                refCyl(context, id, "refStored", sb, "REF stored bottle " ~ (sb + 1),
                        vector(W + rackD / 2, slotY, rackZ + 3 * millimeter),
                        vector(W + rackD / 2, slotY, rackZ + 3 * millimeter + definition.outH + 25 * millimeter),
                        definition.outD / 2);
            }
            refCyl(context, id, "refOut", 0, "REF output bottle",
                    vector(definition.outX, yc, ph + definition.padH),
                    vector(definition.outX, yc, ph + definition.padH + definition.outH),
                    definition.outD / 2);
        }

        // ------------------------- 3D-printed parts -------------------------
        const cupHoleR = (definition.bottleD + 4 * millimeter) / 2;
        const seat = ph + definition.padH;                         // where the output bottle rests
        const tipZ = seat + definition.outH + definition.funnelH + definition.nozGap;   // bottom of the nozzle tip block
        const padRimR = definition.outPadD / 2 + definition.flangeW + 3 * millimeter;
        const g = definition.tubeOD + 1.5 * millimeter;            // tube hole spacing (2 x 2 grid)
        const bw = g + definition.tubeOD + 4 * millimeter;         // nozzle tip block width
        const tipH = 12 * millimeter;
        const plateZ0 = H - t - 4 * millimeter;                    // nozzle mounting plate bottom
        const pilot = 1.4 * millimeter;                            // 2.8 mm pilot for M3 screws into plastic
        if (definition.showPrinted)
        {
            // Diffuser cups: drop into the plinth holes, flange rests on top, thin floor lets the LED glow through
            for (var c = 0; c < n; c += 1)
            {
                const cid = id + ("cup" ~ c);
                const zb = ph - definition.cupDepth - definition.cupFloor;
                const bo = cupHoleR - definition.fitClear;               // square cup outer half-width
                const fo = cupHoleR + definition.flangeW;                // flange half-width
                const io = (definition.bottleD + 1 * millimeter) / 2;    // pocket half-width
                const body = boxBody(context, cid + "body", vector(bx[c] - bo, yc - bo, zb), vector(bx[c] + bo, yc + bo, ph));
                const flange = boxBody(context, cid + "flange", vector(bx[c] - fo, yc - fo, ph), vector(bx[c] + fo, yc + fo, ph + definition.flangeT));
                opBoolean(context, cid + "join", { "tools" : qUnion([body, flange]), "operationType" : BooleanOperationType.UNION });
                const inner = boxBody(context, cid + "inner", vector(bx[c] - io, yc - io, ph - definition.cupDepth),
                        vector(bx[c] + io, yc + io, ph + definition.flangeT + e));
                opBoolean(context, cid + "cut", { "tools" : inner, "targets" : qUnion([body, flange]), "operationType" : BooleanOperationType.SUBTRACTION });
                nameBodies(context, qCreatedBy(cid, EntityType.BODY), "PRINT diffuser cup " ~ (c + 1));
            }

            // Output pad: plug into the output hole, rim with a recess that locates the bottle and catches drips,
            // hollow underneath so LED light reaches the thin floor
            const pid = id + "pad";
            const holeR = definition.outPadD / 2 - definition.fitClear;
            const plug = cylBody(context, pid + "plug", vector(definition.outX, yc, ph - t - 3 * millimeter), vector(definition.outX, yc, seat), holeR);
            const rim = cylBody(context, pid + "rim", vector(definition.outX, yc, ph), vector(definition.outX, yc, seat + 4 * millimeter), padRimR);
            opBoolean(context, pid + "join", { "tools" : qUnion([plug, rim]), "operationType" : BooleanOperationType.UNION });
            const recess = cylBody(context, pid + "recess", vector(definition.outX, yc, seat), vector(definition.outX, yc, seat + 4 * millimeter + e),
                    definition.outD / 2 + 0.5 * millimeter);
            const cavity = cylBody(context, pid + "cavity", vector(definition.outX, yc, ph - t - 3 * millimeter - e),
                    vector(definition.outX, yc, seat - 0.8 * millimeter), holeR - 1.2 * millimeter);
            opBoolean(context, pid + "cut", { "tools" : qUnion([recess, cavity]), "targets" : qUnion([plug, rim]), "operationType" : BooleanOperationType.SUBTRACTION });
            nameBodies(context, qCreatedBy(pid, EntityType.BODY), "PRINT output pad");

            // Nozzle holder: plate screws under the top panel, arm drops down, tip block holds 4 tube ends in a 2 x 2 bundle
            const nid = id + "nozzle";
            const ox = definition.outX;
            const tip = boxBody(context, nid + "tip", vector(ox - bw / 2, yc - bw / 2, tipZ), vector(ox + bw / 2, yc + bw / 2, tipZ + tipH));
            const arm = boxBody(context, nid + "arm", vector(ox + bw / 2 - 1.5 * millimeter, yc - 6 * millimeter, tipZ),
                    vector(ox + bw / 2 + 4 * millimeter, yc + 6 * millimeter, plateZ0 + 1 * millimeter));
            const plate = boxBody(context, nid + "plate", vector(ox - definition.nozDx / 2 - 6 * millimeter, yc - 8 * millimeter, plateZ0),
                    vector(ox + definition.nozDx / 2 + 6 * millimeter, yc + 8 * millimeter, H - t));
            const guideZ = (tipZ + tipH + plateZ0) / 2;
            const guide = boxBody(context, nid + "guide", vector(ox - bw / 2, yc - bw / 2, guideZ), vector(ox + bw / 2, yc + bw / 2, guideZ + 6 * millimeter));
            opBoolean(context, nid + "join", { "tools" : qUnion([tip, arm, plate, guide]), "operationType" : BooleanOperationType.UNION });
            const tsx = [-1, 1, -1, 1];
            const tsy = [-1, -1, 1, 1];
            var holes = [];
            for (var h = 0; h < 4; h += 1)
            {
                holes = append(holes, cylBody(context, nid + ("tube" ~ h),
                            vector(ox + tsx[h] * g / 2, yc + tsy[h] * g / 2, tipZ - e),
                            vector(ox + tsx[h] * g / 2, yc + tsy[h] * g / 2, guideZ + 6 * millimeter + e), (definition.tubeOD + 0.3 * millimeter) / 2));
            }
            holes = append(holes, cylBody(context, nid + "screwL", vector(ox - definition.nozDx / 2, yc, plateZ0 - e), vector(ox - definition.nozDx / 2, yc, H - t + e), pilot));
            holes = append(holes, cylBody(context, nid + "screwR", vector(ox + definition.nozDx / 2, yc, plateZ0 - e), vector(ox + definition.nozDx / 2, yc, H - t + e), pilot));
            opBoolean(context, nid + "cut", { "tools" : qUnion(holes), "targets" : qUnion([tip, arm, plate, guide]), "operationType" : BooleanOperationType.SUBTRACTION });
            nameBodies(context, qCreatedBy(nid, EntityType.BODY), "PRINT nozzle holder");

            // Tube clips: snap the outlet tubes into the grooves from the front
            for (var tc = 0; tc < n; tc += 1)
            {
                const tid = id + ("tubeClip" ~ tc);
                const cx = clipX[tc];
                const blk = boxBody(context, tid + "body", vector(cx - clipW / 2, pwy - clipD, clipZ0), vector(cx + clipW / 2, pwy, clipZ0 + clipH));
                var cuts = [];
                for (var row = 0; row < n; row += 1)
                {
                    const gz = clipZ0 + clipH - 3 * millimeter - (row + 0.5) * clipRow;
                    const gy = pwy - clipD + 0.425 * definition.tubeOD;      // groove center sits behind the face -> narrow snap opening
                    cuts = append(cuts, cylBody(context, tid + ("groove" ~ row),
                                vector(cx - clipW / 2 - e, gy, gz), vector(cx + clipW / 2 + e, gy, gz), definition.tubeOD / 2 + 0.2 * millimeter));
                }
                cuts = append(cuts, cylBody(context, tid + "pilot", vector(cx, pwy - 9 * millimeter, clipZs), vector(cx, pwy + e, clipZs), pilot));
                opBoolean(context, tid + "cut", { "tools" : qUnion(cuts), "targets" : blk, "operationType" : BooleanOperationType.SUBTRACTION });
                nameBodies(context, blk, "PRINT tube clip " ~ (tc + 1));
            }

            // Storage rack (bottle sockets + funnel holder)
            if (nSlots >= 1)
            {
                const rid = id + "rack";
                const rk = boxBody(context, rid + "body", vector(W, rackY0, rackZ), vector(W + rackD, rackY0 + rackL, rackZ + rackH));
                var holes2 = [];
                for (var so = 0; so < nSlots; so += 1)
                {
                    const slotY = rackY0 + 2 * millimeter + (so + 0.5) * rackSp;
                    if (hasFunnel && so == nSlots - 1)
                    {
                        // funnel sits point-down in a through hole; the cone rests on the rim
                        holes2 = append(holes2, cylBody(context, rid + ("slot" ~ so),
                                    vector(W + rackD / 2, slotY, rackZ - e), vector(W + rackD / 2, slotY, rackZ + rackH + e), definition.funnelD * 0.3));
                    }
                    else
                    {
                        holes2 = append(holes2, cylBody(context, rid + ("slot" ~ so),
                                    vector(W + rackD / 2, slotY, rackZ + 3 * millimeter), vector(W + rackD / 2, slotY, rackZ + rackH + e),
                                    definition.outD / 2 + 0.5 * millimeter));
                    }
                }
                for (var rp = 0; rp < size(rackScrewY); rp += 1)
                {
                    holes2 = append(holes2, cylBody(context, rid + ("pilot" ~ rp),
                                vector(W - e, rackScrewY[rp], rackZ + rackH / 2), vector(W + 9 * millimeter, rackScrewY[rp], rackZ + rackH / 2), pilot));
                }
                opBoolean(context, rid + "cut", { "tools" : qUnion(holes2), "targets" : rk, "operationType" : BooleanOperationType.SUBTRACTION });
                nameBodies(context, rk, "PRINT storage rack");
            }

            // Screw blocks: glue into the corners; back panel and bay lid screw into them
            for (var bb2 = 0; bb2 < 4; bb2 += 1)
            {
                const kid = id + ("backBlock" ~ bb2);
                const x0 = backBlocks[bb2][0];
                const z0 = backBlocks[bb2][1];
                const blk = boxBody(context, kid + "body", vector(x0, D - t - bs, z0), vector(x0 + bs, D - t, z0 + bs));
                const hole = cylBody(context, kid + "hole", vector(x0 + bs / 2, D - t - bs - e, z0 + bs / 2), vector(x0 + bs / 2, D - t + e, z0 + bs / 2), pilot);
                opBoolean(context, kid + "cut", { "tools" : hole, "targets" : blk, "operationType" : BooleanOperationType.SUBTRACTION });
                nameBodies(context, blk, "PRINT screw block back " ~ (bb2 + 1));
            }
            for (var lb2 = 0; lb2 < 3; lb2 += 1)
            {
                const kid = id + ("lidBlock" ~ lb2);
                const x0 = lidBlocks[lb2][0];
                const y0 = lidBlocks[lb2][1];
                const blk = boxBody(context, kid + "body", vector(x0, y0, H - t - bs), vector(x0 + bs, y0 + bs, H - t));
                const hole = cylBody(context, kid + "hole", vector(x0 + bs / 2, y0 + bs / 2, H - t - bs - e), vector(x0 + bs / 2, y0 + bs / 2, H - t + e), pilot);
                opBoolean(context, kid + "cut", { "tools" : hole, "targets" : blk, "operationType" : BooleanOperationType.SUBTRACTION });
                nameBodies(context, blk, "PRINT screw block lid " ~ (lb2 + 1));
            }
        }

        // ------------------------- Design checks -------------------------
        var msg = "";
        const liquidZ = ph - definition.cupDepth + definition.bottleH - 20 * millimeter;
        if (tipZ <= liquidZ)
        {
            msg = msg ~ "Siphon risk: nozzle tip (" ~ mm(tipZ) ~ " mm) is below reservoir liquid level (" ~ mm(liquidZ) ~ " mm); raise the output pad. ";
        }
        if (definition.motorL > definition.chaseD - 3 * millimeter)
        {
            msg = msg ~ "Pump motors do not fit in the chase; increase chase depth. ";
        }
        if (definition.pumpHoleD < definition.motorD)
        {
            msg = msg ~ "Pump hole is smaller than the motor. ";
        }
        if (definition.bottlePitch < definition.faceW + 5 * millimeter)
        {
            msg = msg ~ "Pumps overlap; increase bottle pitch. ";
        }
        if (definition.bottlePitch < definition.bottleD + 10 * millimeter)
        {
            msg = msg ~ "Bottles too close; increase bottle pitch. ";
        }
        if (bx[n - 1] + (definition.bottleD + 4 * millimeter) / 2 + 10 * millimeter > definition.outX - definition.outPadD / 2)
        {
            msg = msg ~ "Last bottle crowds the output pad; move output X right. ";
        }
        if (definition.outX + definition.outPadD / 2 > xd - 5 * millimeter)
        {
            msg = msg ~ "Output pad runs into the divider; widen W or move output X. ";
        }
        if (definition.bottleX0 - (definition.bottleD + 4 * millimeter) / 2 < t + 5 * millimeter)
        {
            msg = msg ~ "First bottle is too close to the left wall. ";
        }
        if (ph - definition.cupDepth + definition.bottleH > definition.pumpZ - definition.faceH / 2 - 10 * millimeter)
        {
            msg = msg ~ "Bottle tops are too close to the pumps; raise pump Z. ";
        }
        if (definition.pumpZ + definition.faceH / 2 > H - t - 5 * millimeter)
        {
            msg = msg ~ "Pumps hit the top panel; lower pump Z or raise H. ";
        }

        const sizes = [
            ["Base", W, D], ["Side", D, H], ["Back", W, H],
            ["Top", xd + t, D], ["Bay lid", W - xd - t, D], ["Divider", D, H],
            ["Bay front", W - xd, H], ["Pump wall", xd, H],
            ["Plinth top", xd - t, pd], ["Plinth front", xd, ph]
        ];
        for (var p in sizes)
        {
            if (!fitsBed(p[1], p[2], definition.bedW, definition.bedH))
            {
                msg = msg ~ p[0] ~ " panel is larger than the laser bed. ";
            }
        }

        if (ph - definition.cupDepth - definition.cupFloor < t + 8 * millimeter)
        {
            msg = msg ~ "Diffuser cups reach too close to the LEDs; reduce cup depth or raise the plinth. ";
        }
        if (bx[n - 1] + cupHoleR + definition.flangeW > definition.outX - padRimR)
        {
            msg = msg ~ "Last diffuser cup flange hits the output pad rim; move output X right. ";
        }
        if (definition.outX + padRimR > xd)
        {
            msg = msg ~ "Output pad rim runs into the divider. ";
        }
        const target = definition.funnelD > 0 * millimeter ? definition.funnelD : definition.neckID;
        if (g + definition.tubeOD > target - 4 * millimeter)
        {
            msg = msg ~ "4-tube bundle is too wide for the funnel / bottle neck. ";
        }
        if (tipZ + tipH > plateZ0 - 10 * millimeter)
        {
            msg = msg ~ "Output bottle is too tall for the nozzle holder; raise H or lower the pad. ";
        }

        if (definition.scrW > definition.ebayW - 2 * t - 4 * millimeter)
        {
            msg = msg ~ "Screen board is wider than the electronics bay; widen the bay. ";
        }
        if (definition.scrZ + definition.scrH / 2 > H - t - 12 * millimeter - 2 * millimeter)
        {
            msg = msg ~ "Screen hits the lid screw blocks; lower screen Z. ";
        }
        if (definition.hasSwitch && definition.scrZ - definition.scrH / 2 < 60 * millimeter + definition.swH / 2 + 10 * millimeter)
        {
            msg = msg ~ "Screen is too close to the power switch; raise screen Z. ";
        }
        if (rackL > D)
        {
            msg = msg ~ "Storage rack is longer than the box is deep; store fewer bottles. ";
        }
        if (rackZ + rackH > H)
        {
            msg = msg ~ "Storage rack sits above the top of the box; lower its Z. ";
        }
        if (clipZ0 + clipH > H - t - 3 * millimeter)
        {
            msg = msg ~ "Tube clips hit the top panel; lower pump Z or raise H. ";
        }

        if (definition.scrHoleDx / 2 - 3 * millimeter < abs(definition.winOff) + definition.winW / 2)
        {
            msg = msg ~ "Screen window overlaps the mounting holes; check window offset / size. ";
        }

        if (msg != "")
        {
            reportFeatureWarning(context, id, msg);
        }
    });

// ============================ Helpers ============================

function makePanel(context is Context, id is Id, key is string, name is string, lo is Vector, hi is Vector) returns Query
{
    fCuboid(context, id + key + "body", { "corner1" : lo, "corner2" : hi });
    const q = qCreatedBy(id + key + "body", EntityType.BODY);
    setProperty(context, { "entities" : q, "propertyType" : PropertyType.NAME, "value" : name });
    return q;
}

function cutCyl(context is Context, id is Id, key is string, k is number, target is Query, a is Vector, b is Vector, r is ValueWithUnits)
{
    const toolId = id + (key ~ k) + "tool";
    fCylinder(context, toolId, { "bottomCenter" : a, "topCenter" : b, "radius" : r });
    opBoolean(context, id + (key ~ k) + "cut", {
                "tools" : qCreatedBy(toolId, EntityType.BODY),
                "targets" : target,
                "operationType" : BooleanOperationType.SUBTRACTION
            });
}

function cutBox(context is Context, id is Id, key is string, k is number, target is Query, lo is Vector, hi is Vector)
{
    const toolId = id + (key ~ k) + "tool";
    fCuboid(context, toolId, { "corner1" : lo, "corner2" : hi });
    opBoolean(context, id + (key ~ k) + "cut", {
                "tools" : qCreatedBy(toolId, EntityType.BODY),
                "targets" : target,
                "operationType" : BooleanOperationType.SUBTRACTION
            });
}

function refCyl(context is Context, id is Id, key is string, k is number, name is string, a is Vector, b is Vector, r is ValueWithUnits)
{
    const bodyId = id + (key ~ k);
    fCylinder(context, bodyId, { "bottomCenter" : a, "topCenter" : b, "radius" : r });
    setProperty(context, { "entities" : qCreatedBy(bodyId, EntityType.BODY), "propertyType" : PropertyType.NAME, "value" : name });
}

function refBox(context is Context, id is Id, key is string, k is number, name is string, lo is Vector, hi is Vector)
{
    const bodyId = id + (key ~ k);
    fCuboid(context, bodyId, { "corner1" : lo, "corner2" : hi });
    setProperty(context, { "entities" : qCreatedBy(bodyId, EntityType.BODY), "propertyType" : PropertyType.NAME, "value" : name });
}

function setAxis(v is Vector, axis is number, value is ValueWithUnits) returns Vector
{
    return vector(axis == 0 ? value : v[0], axis == 1 ? value : v[1], axis == 2 ? value : v[2]);
}

// Splits the joint region into an odd number of segments along the joint.
// Odd segments become tabs (cut from the slot panel); even segments, including both ends,
// stay with the slot panel (cut from the tab panel's extension). Subtract-only, so part identities never change.
function fingerJoint(context is Context, id is Id, j is number, tabPanel is Query, slotPanel is Query,
    lo is Vector, hi is Vector, axis is number, fw is ValueWithUnits)
{
    const len = hi[axis] - lo[axis];
    var k = round(len / fw);
    if (k < 3)
    {
        k = 3;
    }
    if (k - 2 * floor(k / 2) == 0)
    {
        k = k + 1;
    }
    const seg = len / k;
    const jid = id + ("joint" ~ j);
    var odd = [];
    var even = [];
    for (var s = 0; s < k; s += 1)
    {
        const a = setAxis(lo, axis, lo[axis] + s * seg);
        const b = setAxis(hi, axis, lo[axis] + (s + 1) * seg);
        const q = boxBody(context, jid + ("s" ~ s), a, b);
        if (s - 2 * floor(s / 2) == 1)
        {
            odd = append(odd, q);
        }
        else
        {
            even = append(even, q);
        }
    }
    opBoolean(context, jid + "slots", { "tools" : qUnion(odd), "targets" : slotPanel, "operationType" : BooleanOperationType.SUBTRACTION });
    opBoolean(context, jid + "trim", { "tools" : qUnion(even), "targets" : tabPanel, "operationType" : BooleanOperationType.SUBTRACTION });
}

function cylBody(context is Context, id is Id, a is Vector, b is Vector, r is ValueWithUnits) returns Query
{
    fCylinder(context, id, { "bottomCenter" : a, "topCenter" : b, "radius" : r });
    return qCreatedBy(id, EntityType.BODY);
}

function boxBody(context is Context, id is Id, lo is Vector, hi is Vector) returns Query
{
    fCuboid(context, id, { "corner1" : lo, "corner2" : hi });
    return qCreatedBy(id, EntityType.BODY);
}

function nameBodies(context is Context, q is Query, name is string)
{
    setProperty(context, { "entities" : q, "propertyType" : PropertyType.NAME, "value" : name });
}

function fitsBed(a is ValueWithUnits, b is ValueWithUnits, bw is ValueWithUnits, bh is ValueWithUnits) returns boolean
{
    return (a <= bw && b <= bh) || (a <= bh && b <= bw);
}

function mm(x is ValueWithUnits) returns number
{
    return roundToPrecision(x / millimeter, 0);
}
