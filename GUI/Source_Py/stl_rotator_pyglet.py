import sys
import pyglet
from pyglet.gl import *
from stl import mesh
import numpy as np

if len(sys.argv) < 2:
    print("Uso: python stl_rotator_pyglet.py archivo.stl")
    sys.exit(1)

filename = sys.argv[1]
model = mesh.Mesh.from_file(filename)

# Normalizamos el modelo
min_, max_ = model.min_, model.max_
center = (min_ + max_) / 2
scale = 2.0 / np.max(max_ - min_)

vertices = (model.vectors - center) * scale
vertices = vertices.reshape(-1, 3).astype('f')

window = pyglet.window.Window(width=800, height=600, caption="STL Viewer", resizable=True)
rotation = [0, 0]

@window.event
def on_draw():
    window.clear()
    glEnable(GL_DEPTH_TEST)
    glLoadIdentity()
    glTranslatef(0, 0, -5)
    glRotatef(rotation[0], 1, 0, 0)
    glRotatef(rotation[1], 0, 1, 0)

    glBegin(GL_TRIANGLES)
    for v in vertices:
        glVertex3f(v[0], v[1], v[2])
    glEnd()

def update(dt):
    rotation[0] += 20 * dt
    rotation[1] += 30 * dt

pyglet.clock.schedule(update)
pyglet.app.run()
