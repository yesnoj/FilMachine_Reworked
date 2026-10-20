"""Anteprime PNG dell'assieme con VTK (fuori schermo): ombreggiatura, occlusione ambientale, spigoli vivi in nero."""
import os, math
import numpy as np
import vtk
from vtk.util import numpy_support as ns
import matplotlib.colors as mc
from common import OUT

_CACHE = {}

def polydata(shape, tol=0.2, ang=0.3):
    v, t = shape.val().tessellate(tol, ang)
    pts = vtk.vtkPoints(); pts.SetData(ns.numpy_to_vtk(np.array([[p.x, p.y, p.z] for p in v], dtype=np.float64), deep=True))
    tri = np.array(t, dtype=np.int64); cells = np.hstack([np.full((len(tri), 1), 3, dtype=np.int64), tri]).ravel()
    ca = vtk.vtkCellArray(); ca.SetCells(len(tri), ns.numpy_to_vtkIdTypeArray(cells, deep=True))
    pd = vtk.vtkPolyData(); pd.SetPoints(pts); pd.SetPolys(ca)
    return pd

def render(P, name, azim=-60.0, elev=28.0, hide=(), size=(1700, 1250), color_of=None, zoom=1.25, edges=True, tol=0.2, focus=None, opacity=None):
    """P: dizionario nome -> forma. azim/elev: direzione da cui si guarda (gradi). hide: prefissi da non disegnare.
    opacity: dizionario prefisso -> opacita' (per mostrare l'interno)."""
    ren = vtk.vtkRenderer(); ren.SetBackground(1, 1, 1)
    for n, p in P.items():
        if any(n.startswith(h) for h in hide): continue
        if (n, tol) not in _CACHE: _CACHE[(n, tol)] = polydata(p, tol)
        pd = _CACHE[(n, tol)]
        nrm = vtk.vtkPolyDataNormals(); nrm.SetInputData(pd); nrm.SetFeatureAngle(35); nrm.SplittingOn(); nrm.ConsistencyOff(); nrm.Update()
        m = vtk.vtkPolyDataMapper(); m.SetInputConnection(nrm.GetOutputPort())
        a = vtk.vtkActor(); a.SetMapper(m)
        col = mc.to_rgb(color_of(n) if color_of else "#b0b4ba")
        pr = a.GetProperty(); pr.SetColor(*col); pr.SetAmbient(0.25); pr.SetDiffuse(0.75); pr.SetSpecular(0.12); pr.SetSpecularPower(20)
        if opacity:
            for k, o in opacity.items():
                if n.startswith(k): pr.SetOpacity(o)
        ren.AddActor(a)
        if edges:
            fe = vtk.vtkFeatureEdges(); fe.SetInputData(pd); fe.BoundaryEdgesOn(); fe.FeatureEdgesOn(); fe.SetFeatureAngle(40); fe.ManifoldEdgesOff(); fe.NonManifoldEdgesOff(); fe.ColoringOff()
            em = vtk.vtkPolyDataMapper(); em.SetInputConnection(fe.GetOutputPort()); em.SetResolveCoincidentTopologyToPolygonOffset()
            ea = vtk.vtkActor(); ea.SetMapper(em); ea.GetProperty().SetColor(0.12, 0.12, 0.12); ea.GetProperty().SetLineWidth(1.0)
            if opacity and any(n.startswith(k) for k in opacity): ea.GetProperty().SetOpacity(0.35)
            ren.AddActor(ea)
    win = vtk.vtkRenderWindow(); win.SetOffScreenRendering(1); win.AddRenderer(ren); win.SetSize(*size); win.SetMultiSamples(8)
    ren.ResetCamera()
    cam = ren.GetActiveCamera()
    b = ren.ComputeVisiblePropBounds(); c = focus or ((b[0] + b[1]) / 2, (b[2] + b[3]) / 2, (b[4] + b[5]) / 2)
    d = 3.0 * max(b[1] - b[0], b[3] - b[2], b[5] - b[4])
    az, el = math.radians(azim), math.radians(elev)
    cam.SetFocalPoint(*c); cam.SetPosition(c[0] + d * math.cos(el) * math.cos(az), c[1] + d * math.cos(el) * math.sin(az), c[2] + d * math.sin(el))
    cam.SetViewUp(0, 0, 1); cam.SetViewAngle(18)
    ren.ResetCamera(); cam.Zoom(zoom); ren.ResetCameraClippingRange()
    try:
        ren.SetUseSSAO(True); ren.SetSSAORadius(25.0); ren.SetSSAOBias(1.0); ren.SetSSAOKernelSize(128); ren.SSAOBlurOn()
    except Exception:
        pass
    if opacity:
        ren.SetUseDepthPeeling(1); ren.SetMaximumNumberOfPeels(8); win.SetAlphaBitPlanes(1); win.SetMultiSamples(0)
    win.Render()
    f = vtk.vtkWindowToImageFilter(); f.SetInput(win); f.Update()
    path = os.path.join(OUT, "png", name + ".png")
    w = vtk.vtkPNGWriter(); w.SetFileName(path); w.SetInputConnection(f.GetOutputPort()); w.Write()
    return path
