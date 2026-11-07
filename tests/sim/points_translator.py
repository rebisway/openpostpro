# Example translator of the simulator guide: a program is a list of points, one per line: "X Y Z" and an optional feed rate.
# The first point is reached by a rapid move, the next ones by moves at the feed rate (600 mm/min by default).
# A line which starts with # is a comment.
class Translator:
    def start(self, options):
        self.first = True
        return {'unit': 'mm'}

    def next(self, line):
        line = line.strip()
        if not line or line.startswith('#'):
            return []
        try:
            values = [float(v) for v in line.replace(',', ' ').split()]
        except ValueError:
            return [{'cmd': 'message', 'level': 'error', 'text': 'not numbers: ' + line}]
        if len(values) < 3:
            return [{'cmd': 'message', 'level': 'warning', 'text': 'a point needs X Y Z: ' + line}]
        x, y, z = values[:3]
        if self.first:
            self.first = False
            return [{'cmd': 'rapid', 'x': x, 'y': y, 'z': z}]
        feed = values[3] if len(values) > 3 else 600.0
        return [{'cmd': 'linear', 'x': x, 'y': y, 'z': z, 'f': feed}]

    def close(self):
        pass
