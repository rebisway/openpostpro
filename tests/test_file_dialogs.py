# File properties (get_property_file) and the file dialogs of the engine (open_file, save_file), without displaying a window.
import os, sys
sys.dont_write_bytecode = True
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..', 'openpostpro', 'script'))
sys.path.insert(0, os.path.join(here, '..', 'openpostpro', 'postpro'))
import cadcore, postcore
import svg_import, svg_export, gerber_import, airfoil_file, potrace, hexagon
import generic_mill


# ---- a script without file property: None
core = cadcore.Cadcore()
script = hexagon.Cadscript()
script.set_language('en-us')
core.set_script(script)
script.set_core(core)
assert core.get_property_file(0) is None

# ---- the file properties of the bundled scripts: ('open' or 'save', name of the filter, pattern)
for module, filter_name, pattern in ((svg_import, 'SVG', '*.svg'), (airfoil_file, 'Airfoil', '*.dat;*.txt')):
    script = module.Cadscript()
    script.set_language('en-us')
    core = cadcore.Cadcore()
    core.set_script(script)
    script.set_core(core)
    assert core.get_property_file(0) == ('open', filter_name, pattern), (module.__name__, core.get_property_file(0))
    assert core.get_property_file(1) is None, module.__name__
    # the property is a text: the dialog box of the application can edit it
    assert isinstance(core.get_property(0), str)

script = gerber_import.Cadscript()
script.set_language('en-us')
core = cadcore.Cadcore()
core.set_script(script)
script.set_core(core)
mode, name, pattern = core.get_property_file(0)
assert mode == 'open' and '*.gbr' in pattern and '*.drl' in pattern, core.get_property_file(0)

# ---- the post-processor core forwards the same method
post = generic_mill.Postpro()
postcore_object = postcore.Postcore()
postcore_object.set_postpro(post)
assert postcore_object.get_property_file(0) is None
post.get_property_file = lambda index: ('save', 'Text', '*.txt') if index == 2 else None
assert postcore_object.get_property_file(2) == ('save', 'Text', '*.txt')


# ---- open_file and save_file give the choice of the dialog, '' when there is none
class Recorder(cadcore.Cadcore):
    def __init__(self):
        super().__init__()
        self.calls = []

    def _file_dialog(self, save, title, filters, initial, extension):
        self.calls.append((save, title, filters, initial, extension))
        return 'C:/x/y.svg'


core = Recorder()
assert core.open_file('Title', [('SVG', '*.svg')], 'C:/x/a.svg') == 'C:/x/y.svg'
assert core.save_file('Save', [('SVG', '*.svg')], '', '.svg') == 'C:/x/y.svg'
assert core.calls == [(False, 'Title', [('SVG', '*.svg')], 'C:/x/a.svg', ''), (True, 'Save', [('SVG', '*.svg')], '', '.svg')], core.calls

# the scripts ask the dialog of the engine
core = Recorder()
script = svg_import.Cadscript()
script.set_language('en-us')
script.set_core(core)
assert script.ask_file() == 'C:/x/y.svg'
assert core.calls[0][0] is False and core.calls[0][2][0] == ('SVG', '*.svg'), core.calls
core = Recorder()
exporter = svg_export.Cadscript()
exporter.set_language('en-us')
exporter.set_core(core)
assert exporter.ask_file() == 'C:/x/y.svg'
assert core.calls[0][0] is True and core.calls[0][4] == '.svg', core.calls

# a dialog which can not be displayed (no screen, no tkinter) gives '': the script goes on without a file
class NoWindow(cadcore.Cadcore):
    pass


import builtins
real_import = builtins.__import__


def refuse_tkinter(name, *args, **kwargs):
    if name == 'tkinter':
        raise ImportError('no tkinter')
    return real_import(name, *args, **kwargs)


builtins.__import__ = refuse_tkinter
try:
    core = NoWindow()
    script = hexagon.Cadscript()
    script.set_language('en-us')
    core.set_script(script)
    assert core.open_file('t') == ''
    assert core.save_file('t') == ''
finally:
    builtins.__import__ = real_import
print('ok')
