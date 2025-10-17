import os
import datetime
import math
import time
import numpy as np
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.widgets import Button, TextBox

# Use TkAgg or default interactive backend
matplotlib.use('TkAgg')


def haversine_distance_km(lat1, lon1, lat2, lon2):
    """Return the great-circle distance between two points on Earth (in km).

    Uses the haversine formula and assumes a spherical Earth which is
    sufficient for moderate precision here.
    """
    R = 6371.0088  # mean radius of Earth in km
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    dphi = math.radians(lat2 - lat1)
    dlambda = math.radians(lon2 - lon1)

    a = math.sin(dphi / 2.0) ** 2 + math.cos(phi1) * \
        math.cos(phi2) * math.sin(dlambda / 2.0) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return R * c


class InteractiveRadiusSelector:
    """Interactive selector that follows the cursor with a geodesic circle.

    Features:
    - A central point follows the mouse while not locked.
    - Circle radius in km (input box) used to light up points within the circle.
    - Points belonging to the same ObjectID follow the leader's highlight state.
    - Enter locks the center (stops following); Esc unlocks.
    - Save button writes only the highlighted points to a timestamped CSV.
    """

    def __init__(self, csv_path, default_radius_km=100.0):
        if not os.path.exists(csv_path):
            raise FileNotFoundError(f"Input CSV not found: {csv_path}")

        self.df = pd.read_csv(csv_path)
        # Ensure required columns exist
        for col in ['ObjectID', 'Latitude', 'Longitude']:
            if col not in self.df.columns:
                raise ValueError(f"Required column '{col}' missing from CSV")

        self.radius_km = float(default_radius_km)

        # UI / state
        self.locked = False
        self.center = None  # (lat, lon)

        # Map ObjectID -> state (True if highlighted)
        self.unique_oids = self.df['ObjectID'].unique()
        self.object_state = dict.fromkeys(self.unique_oids, False)

        # Precompute lat/lon arrays for speed (and radians for vectorized haversine)
        self.lats = self.df['Latitude'].to_numpy(dtype=float)
        self.lons = self.df['Longitude'].to_numpy(dtype=float)
        self.oids = self.df['ObjectID'].to_numpy()
        self.lats_rad = np.deg2rad(self.lats)
        self.lons_rad = np.deg2rad(self.lons)

        # Map ObjectID -> numpy indices (for quick group operations)
        self.oid_to_indices = {oid: np.nonzero(self.oids == oid)[0]
                               for oid in self.unique_oids}

        # Setup plot
        self.fig, self.ax = plt.subplots(figsize=(10, 7))
        self.ax.set_title(
            'Interactive Radius Selector - move mouse to position center, Enter to lock, Esc to unlock')
        self.ax.set_xlabel('Longitude')
        self.ax.set_ylabel('Latitude')
        self.ax.grid(True)

        # colors: highlighted vs dimmed
        self.highlight_color = np.array([1.0, 0.2, 0.2, 1.0])  # red opaque
        self.dim_color = np.array([0.6, 0.6, 0.6, 0.3])

        # initial scatter: prepare facecolor buffer and set it once
        n = len(self.lats)
        self.facecolors = np.tile(self.dim_color, (n, 1))
        self.scatter = self.ax.scatter(self.lons, self.lats,
                                       c=self.facecolors, s=40, edgecolors='k')

        # performance: throttle visual updates to avoid excessive redraws
        self._last_update = 0.0
        self._min_update_interval = 0.03  # seconds (about 33 FPS max)

        # central marker and circle patch
        self.center_marker, = self.ax.plot(
            [], [], marker='x', color='black', markersize=10, zorder=5)
        self.circle_patch = plt.Circle(
            (0, 0), 0, transform=self.ax.transData, fill=False, color='blue', linewidth=1.5, alpha=0.7, zorder=4)
        self.ax.add_patch(self.circle_patch)

        # Widgets: radius textbox and save button
        axbox = plt.axes([0.02, 0.92, 0.15, 0.05])
        self.text_box = TextBox(axbox, 'Radius (km)',
                                initial=str(self.radius_km))
        self.text_box.on_submit(self._on_radius_submit)

        axsave = plt.axes([0.2, 0.92, 0.1, 0.05])
        self.save_button = Button(axsave, 'Save')
        self.save_button.on_clicked(self._on_save)

        # Connect events
        self.fig.canvas.mpl_connect('motion_notify_event', self._on_motion)
        self.fig.canvas.mpl_connect('key_press_event', self._on_key)

        # Set axis limits nicely around data
        margin = 0.02
        lon_min, lon_max = np.min(self.lons), np.max(self.lons)
        lat_min, lat_max = np.min(self.lats), np.max(self.lats)
        self.ax.set_xlim(lon_min - margin, lon_max + margin)
        self.ax.set_ylim(lat_min - margin, lat_max + margin)

    def _on_radius_submit(self, text):
        try:
            val = float(text)
            if val <= 0:
                raise ValueError('Radius must be positive')
            self.radius_km = val
            # Update circle visuals if center exists
            if self.center is not None:
                self._update_circle_patch()
                self._update_highlights()
        except Exception as e:
            print(f'Invalid radius value: {e}')

    def _on_save(self, event):
        # Save only the highlighted points (object_state True)
        mask = np.array([self.object_state[oid] for oid in self.oids])
        saved_df = self.df.loc[mask].copy()
        timestamp = datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f'highlighted_points_{timestamp}.csv'
        saved_df.to_csv(filename, index=False)
        print(f'Saved {len(saved_df)} highlighted points to {filename}')

    def _on_motion(self, event):
        # Update center to mouse position unless locked
        if event.inaxes != self.ax:
            return

        if not self.locked:
            # Matplotlib x is longitude, y is latitude
            mx, my = event.xdata, event.ydata
            if mx is None or my is None:
                return
            # Throttle updates to reduce CPU when mouse moves rapidly
            now = time.time()
            if now - self._last_update < self._min_update_interval:
                # still update the center marker for visual responsiveness but skip heavy work
                self.center = (my, mx)
                self.center_marker.set_data([mx], [my])
                return

            self._last_update = now
            self.center = (my, mx)
            self.center_marker.set_data([mx], [my])
            self._update_circle_patch()
            self._update_highlights()
            # use draw_idle to batch with the GUI main loop
            self.fig.canvas.draw_idle()

    def _on_key(self, event):
        # Enter locks, Esc unlocks
        if event.key is None:
            return
        key = event.key.lower()

        # Lock/unlock
        if key in ('enter', 'return'):
            self.locked = True
            print('Center locked')
            return
        if key == 'escape':
            self.locked = False
            print('Center unlocked')
            return

        # Zoom handling: accept several ctrl variants
        zoom_in_keys = {'ctrl++', 'ctrl+=',
                        'ctrl+\u003d', 'ctrl+\u002b', '++', 'ctrl++/'}
        zoom_out_keys = {'ctrl+-', 'ctrl+_', 'ctrl+\u002d', '--'}

        if key in zoom_in_keys:
            self._zoom(1.2, event)
        elif key in zoom_out_keys:
            self._zoom(1 / 1.2, event)

    def _zoom(self, scale, event=None):
        """Zoom the axes by a scale factor.

        scale > 1 zooms in, scale < 1 zooms out.
        If event contains xdata/ydata (mouse over axes), zoom around that point. Otherwise use axes center.
        """
        ax = self.ax

        xlim = ax.get_xlim()
        ylim = ax.get_ylim()

        if event is not None and hasattr(event, 'xdata') and event.xdata is not None and event.ydata is not None:
            cx, cy = event.xdata, event.ydata
        else:
            cx = (xlim[0] + xlim[1]) / 2.0
            cy = (ylim[0] + ylim[1]) / 2.0

        new_width = (xlim[1] - xlim[0]) / scale
        new_height = (ylim[1] - ylim[0]) / scale

        new_x0 = cx - (cx - xlim[0]) / scale
        new_x1 = new_x0 + new_width

        new_y0 = cy - (cy - ylim[0]) / scale
        new_y1 = new_y0 + new_height

        ax.set_xlim(new_x0, new_x1)
        ax.set_ylim(new_y0, new_y1)
        self.fig.canvas.draw_idle()

    def _update_circle_patch(self):
        # Approximate circle in lon/lat by drawing an actual circle in axis coords is wrong because of projection/distortion.
        # Instead, compute a polygon of points on the geodesic circle and set the patch to that path by setting radius via transform.
        if self.center is None:
            return

        latc, lonc = self.center

        # Generate many points around the circle (in lat/lon) using bearing-based formula
        # fewer segments is faster; 90 provides smooth enough circle for interactivity
        num = 90
        angles = np.linspace(0, 2 * math.pi, num)

        # Angular distance
        R = 6371.0088
        d = self.radius_km / R

        lats = []
        lons = []
        latc_rad = math.radians(latc)
        lonc_rad = math.radians(lonc)
        for a in angles:
            bearing = a
            lat_rad = math.asin(math.sin(
                latc_rad) * math.cos(d) + math.cos(latc_rad) * math.sin(d) * math.cos(bearing))
            lon_rad = lonc_rad + math.atan2(math.sin(bearing) * math.sin(d) * math.cos(
                latc_rad), math.cos(d) - math.sin(latc_rad) * math.sin(lat_rad))
            lats.append(math.degrees(lat_rad))
            lons.append(math.degrees(lon_rad))

        # Update circle_patch by setting its path via set_xy when it's a Polygon-like object. Circle is not ideal; instead replace with a Line2D region via
        # reusing the patch by setting center and radius in data coords approximately by bounding box mean radius in lon/lat. Simpler: set patch center in data coords
        # and set an approximate radius in lon-lat by computing max distance in lon/lat to polygon points.
        poly_x = np.array(lons)
        poly_y = np.array(lats)

        # Update or create a Line2D to draw the geodesic circle polygon
        if hasattr(self, 'circle_line'):
            self.circle_line.set_data(poly_x, poly_y)
        else:
            (self.circle_line,) = self.ax.plot(poly_x, poly_y,
                                               color='blue', linewidth=1.5, alpha=0.6, zorder=3)

    def _update_highlights(self):
        if self.center is None:
            return
        latc, lonc = self.center

        # Vectorized haversine calculation using precomputed radians arrays
        R = 6371.0088
        latc_rad = math.radians(latc)
        lonc_rad = math.radians(lonc)

        dphi = self.lats_rad - latc_rad
        dlambda = self.lons_rad - lonc_rad
        sin_dphi2 = np.sin(dphi * 0.5)
        sin_dlambda2 = np.sin(dlambda * 0.5)
        a = sin_dphi2 * sin_dphi2 + \
            np.cos(latc_rad) * np.cos(self.lats_rad) * \
            (sin_dlambda2 * sin_dlambda2)
        c = 2 * np.arctan2(np.sqrt(a), np.sqrt(1 - a))
        dists = R * c

        inside_mask = dists <= (self.radius_km + 1e-9)

        # Update object_state per unique ObjectID and mutate facecolors in-place for indices that changed
        changed = False
        for oid, idxs in self.oid_to_indices.items():
            should_highlight = bool(np.any(inside_mask[idxs]))
            if self.object_state[oid] != should_highlight:
                self.object_state[oid] = should_highlight
                changed = True
                if should_highlight:
                    self.facecolors[idxs, :] = self.highlight_color
                else:
                    self.facecolors[idxs, :] = self.dim_color

        # Only update the scatter if something changed (minimize GPU/renderer work)
        if changed:
            self.scatter.set_facecolors(self.facecolors)
            try:
                # Update edges only when necessary (cheap) — keep it minimal
                edge_on = np.array([self.object_state[oid]
                                   for oid in self.oids])
                edge_colors = np.where(edge_on[:, None], np.array(
                    [[0, 0, 0, 1]]), np.array([[0.3, 0.3, 0.3, 0.4]]))
                self.scatter.set_edgecolors(edge_colors.reshape((-1, 4)))
            except Exception:
                pass

    def show(self):
        plt.show()


if __name__ == '__main__':
    # Default input from original script
    INPUT_CSV_FILE = '/home/aryrk/Downloads/TrackDataset_Follow_Rules-GroundTruth(1).csv'
    app = InteractiveRadiusSelector(INPUT_CSV_FILE, default_radius_km=100.0)
    app.show()