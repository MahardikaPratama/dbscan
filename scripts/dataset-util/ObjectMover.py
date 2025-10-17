import os
import datetime
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import matplotlib
# This line tells Matplotlib to use a backend that can create interactive windows.
matplotlib.use('TkAgg')


class BoatTracker:
    """
    An interactive boat tracking application that reads boat locations from a CSV,
    allows users to drag a boat (and its associated group) to a new location,
    and saves the updated positions to a new timestamped CSV file.
    """

    def __init__(self, csv_path):
        """
        Initializes the application, loads data, and sets up the plot.

        Args:
            csv_path (str): The path to the input CSV file.
        """
        if not os.path.exists(csv_path):
            print(f"Error: The file '{csv_path}' was not found.")
            self.df = pd.DataFrame(
                columns=['ObjectID', 'Latitude', 'Longitude'])
            self.is_data_loaded = False
        else:
            self.df = pd.read_csv(csv_path)
            self.is_data_loaded = True

        self.original_df = self.df.copy()

        # --- State variables for robust drag-and-drop interaction ---
        self.selected_index = None
        self.selected_oid = None
        self.group_indices = None
        self.drag_start_pos = None
        self.is_dragging = False  # Flag to differentiate a click from a drag

        # --- Plotting Setup ---
        self.fig, self.ax = plt.subplots(figsize=(12, 9))
        self.ax.set_title(
            "Boat Positions - Click and Drag a Boat to Move its Group")
        self.ax.set_xlabel("Longitude")
        self.ax.set_ylabel("Latitude")
        self.ax.grid(True)

        if self.is_data_loaded:
            # --- Coloring Logic: Assign a consistent color for each unique ObjectID ---
            self.object_ids = self.df['ObjectID'].unique()
            # Using a perceptually uniform colormap is better for distinguishing categories
            self.colors = plt.cm.viridis(
                np.linspace(0, 1, len(self.object_ids)))
            self.oid_color_map = dict(zip(self.object_ids, self.colors))

            point_colors = self.df['ObjectID'].map(self.oid_color_map)

            self.scatter = self.ax.scatter(
                self.df['Longitude'], self.df['Latitude'],
                c=point_colors, picker=5, s=50  # s makes points larger
            )
        else:
            self.ax.text(0.5, 0.5, "No data loaded.", ha='center',
                         va='center', transform=self.ax.transAxes)

        self.connect_events()

    def connect_events(self):
        """Connects matplotlib event listeners to their handler methods."""
        self.fig.canvas.mpl_connect('pick_event', self.on_pick)
        self.fig.canvas.mpl_connect('motion_notify_event', self.on_motion)
        self.fig.canvas.mpl_connect('button_release_event', self.on_release)
        # Key press handler for zooming (Ctrl + / Ctrl -)
        self.fig.canvas.mpl_connect('key_press_event', self.on_key_press)

    def on_pick(self, event):
        """Handles clicking on a boat. This sets up a potential drag."""
        if not hasattr(self, 'scatter') or event.artist != self.scatter:
            return

        if not len(event.ind):
            return

        self.selected_index = event.ind[0]
        self.selected_oid = self.df.loc[self.selected_index, 'ObjectID']
        self.group_indices = self.df[self.df['ObjectID']
                                     == self.selected_oid].index

        mouse_event = event.mouseevent
        self.drag_start_pos = (mouse_event.xdata, mouse_event.ydata)

        print(f"Selected boat group: {self.selected_oid}")

    def on_motion(self, event):
        """Handles mouse movement while the button is pressed."""
        if self.selected_index is None or event.button != 1 or event.xdata is None:
            return

        # A motion event after a pick confirms we are dragging
        self.is_dragging = True

        dx = event.xdata - self.drag_start_pos[0]
        dy = event.ydata - self.drag_start_pos[1]

        # Create a writable copy of all point positions
        current_positions = self.scatter.get_offsets().copy()

        # Update the positions for every boat in the selected group
        for idx in self.group_indices:
            original_lon = self.original_df.loc[idx, 'Longitude']
            original_lat = self.original_df.loc[idx, 'Latitude']

            new_lon = original_lon + dx
            new_lat = original_lat + dy

            current_positions[idx] = [new_lon, new_lat]
            self.df.loc[idx, 'Longitude'] = new_lon
            self.df.loc[idx, 'Latitude'] = new_lat

        # Visually update the plot with the new positions
        self.scatter.set_offsets(current_positions)
        self.fig.canvas.draw_idle()

    def on_release(self, event):
        """Handles releasing the mouse button."""
        # Only save and finalize if a drag actually occurred.
        if self.is_dragging:
            print(f"Movement of group '{self.selected_oid}' finished.")
            self.save_to_csv()
            # Update the original state to be the new current state for the next drag
            self.original_df = self.df.copy()
        else:
            # This handles the case of a simple click without a drag
            print("Selection released (no drag detected).")

        # Reset all state variables for the next interaction
        self.selected_index = None
        self.selected_oid = None
        self.group_indices = None
        self.drag_start_pos = None
        self.is_dragging = False

    def on_key_press(self, event):
        """Handle key press events for zooming.

        Ctrl-+ (or Ctrl/=) will zoom in, Ctrl-- will zoom out.
        If mouse is over the axes, zoom around the mouse position. Otherwise zoom about axes center.
        """
        # Matplotlib reports ctrl modifier in event.key like 'ctrl++' or 'ctrl++'
        if event.key is None:
            return

        key = event.key.lower()

        # Accept several variants: 'ctrl++', 'ctrl+\u003d' (equal), 'ctrl+\u002b'
        zoom_in_keys = {'ctrl++', 'ctrl+=', 'ctrl+\u003d', 'ctrl+\u002b'}
        zoom_out_keys = {'ctrl+-', 'ctrl+_', 'ctrl+\u002d'}

        if key in zoom_in_keys:
            self.zoom(1.2, event)
        elif key in zoom_out_keys:
            self.zoom(1/1.2, event)

    def zoom(self, scale, event=None):
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

        # Compute new limits
        new_width = (xlim[1] - xlim[0]) / scale
        new_height = (ylim[1] - ylim[0]) / scale

        new_x0 = cx - (cx - xlim[0]) / scale
        new_x1 = new_x0 + new_width

        new_y0 = cy - (cy - ylim[0]) / scale
        new_y1 = new_y0 + new_height

        ax.set_xlim(new_x0, new_x1)
        ax.set_ylim(new_y0, new_y1)
        self.fig.canvas.draw_idle()

    def save_to_csv(self):
        """Saves the current DataFrame to a new CSV file with a timestamp."""
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"TrackDataset_moved_{timestamp}.csv"

        try:
            self.df.to_csv(filename, index=False)
            print(f"Successfully saved new positions to '{filename}'")
        except Exception as e:
            print(f"Error saving file: {e}")

    def run(self):
        """Displays the plot and starts the application."""
        if self.is_data_loaded:
            print("Application started. You can now interact with the plot.")
        else:
            print("Application started, but no data was loaded due to missing file.")
        plt.show()


if __name__ == '__main__':
    # The name of the CSV file you uploaded
    INPUT_CSV_FILE = '/home/mahardika-pratama/drive/project/dbscan/input/scenario-1/TrackDataset_Boat.csv'

    app = BoatTracker(INPUT_CSV_FILE)
    app.run()