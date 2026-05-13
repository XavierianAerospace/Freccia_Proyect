# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

a = Analysis(
    ['client_viewer.py'],
    pathex=[],
    binaries=[],
    datas=[
        ('Map', 'Map'),
        ('Map/assets', 'Map/assets'),
    ],
    hiddenimports=[
        'PyQt6.QtWebEngineWidgets',
        'PyQt6.QtWebEngineCore',
        'PyQt6.QtWebEngineQuick',
        'PyQt6.QtQml',
        'pyqtgraph.opengl',
        'numpy',
        'trimesh',
        'stl',
        'socket',
        'threading',
        'tempfile'
    ],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_scripts, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='FRECCIA_XAE',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon='Map/assets/logo_xae.png',
    distpath='.'
)