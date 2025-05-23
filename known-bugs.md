# OpenTTD's known bugs

## Table of contents

- 1.0) About
- 2.0) Known bugs

## 1.0) About

All bugs listed below are marked as known. Please do not submit any bugs
that are the same as these. If you do, do not act surprised, because
we WILL flame you!

The current list of known bugs that we intend to fix can be found in our
bug tracking system at https://github.com/OpenTTD/OpenTTD/issues
Also check the closed bugs when searching for your bug in this system as we
might have fixed the bug in the mean time.

## 2.0) Known bugs

This section lists all known bugs that we do not intend to fix and the
reasons why we think that fixing them is infeasible. We might make some
minor improvements that reduce the scope of these bugs, but we will not
be able to completely fix them.

### No suitable AI can be found:

If you have no AIs and an AI is started the so-called 'dummy' AI will
be loaded. This AI does nothing but writing a message on the AI debug
window and showing a red warning. There are basically two solutions
for this problem: Either you set the number of AI players to 0 so that
no AI is started. You find that setting at the top of the window in the
"AI / Game Scripts Settings" window.

The other solution is acquiring (downloading) some AI. The easiest way
to do this is via the "Check Online Content" button in the main (intro)
menu or directly in the "AI / Game Scripts Settings" dialogue via the
"Check Online Content" button.

### After a while of playing, colours get corrupted:

In Windows 7 the background slideshow corrupts the colour mapping
of OpenTTD's 8bpp screen modes. Workarounds for this are:

* Switching to windowed mode, instead of fullscreen
* Switching off background slideshow
* Setting up the `32bpp-anim` or `32bpp-optimized` blitter

### Custom vehicle type name is incorrectly aligned:

Some NewGRFs use sprites that are bigger than normal in the "buy
vehicle" window. Due to this they have to encode an offset for
the vehicle type name. Upon renaming the vehicle type this encoded
offset is stripped from the name because the "edit box" cannot show
this encoding. As a result the custom vehicle type names will get
the default alignment. The only way to (partially) fix this is by
adding spaces to the custom name.

### Clipping problems [#119]:

In some cases sprites are not drawn as one would expect. Examples of
this are aircraft that might be hidden below the runway or trees that
in some cases are rendered over vehicles.

The primary cause of this problem is that OpenTTD does not have enough
data (like a 3D model) to properly determine what needs to be drawn in
front of what. OpenTTD has bounding boxes but in lots of cases they
are either too big or too small and then cause problems with what
needs to be drawn in front of what. Also some visual tricks are used.

For example trains at 8 pixels high, the catenary needs to be drawn
above that. When you want to draw bridges on top of that, which are
only one height level (= 8 pixels) higher, you are getting into some
big problems.

We can not change the height levels; it would require us to either
redraw all vehicle or all landscape graphics. Doing so would mean we
leave the Transport Tycoon graphics, which in effect means OpenTTD
will not be a Transport Tycoon clone anymore.

### Mouse scrolling not possible at the edges of the screen [#383] [#3966]:

Scrolling the viewport with the mouse cursor at the edges of the screen
in the same direction of the edge will fail. If the cursor is near the
edge the scrolling will be very slow.

OpenTTD only receives cursor position updates when the cursor is inside
OpenTTD's window. It is not told how far you have moved the cursor
outside of OpenTTD's window.

### Lost trains ignore (block) exit signals [#1473]:

If trains are lost they ignore block exit signals, blocking junctions
with presignals. This is caused because the path finders cannot tell
where the train needs to go. As such a random direction is chosen at
each junction. This causes the trains to occasionally to make choices
that are unwanted from a player's point of view.

This will not be fixed because lost trains are in almost all cases a
network problem, e.g. a train can never reach a specific place. This
makes the impact of fixing the bug enormously small against the amount
of work needed to write a system that prevents the lost trains from
taking the wrong direction.

### Vehicle owner of last transfer leg gets paid for all [#2427]:

When you make a transfer system that switches vehicle owners. This
is only possible with 'industry stations', e.g. the oil rig station
the owner of the vehicle that does the final delivery gets paid for
the whole trip. It is not shared amongst the different vehicle
owners that have participated in transporting the cargo.

This sharing is not done because it would enormously increase the
memory and CPU usage in big games for something that is happening
in only one corner case. We think it is not worth the effort until
sharing of stations is an official feature.

### Forbid 90 degree turns does not work for crossing PBS paths [#2737]:

When you run a train through itself on a X junction with PBS turned on
the train will not obey the 'forbid 90 degree turns' setting. This is
due to the fact that we can not be sure that the setting was turned
off when the track was reserved, which means that we assume it was
turned on and that the setting does not hold at the time. We made it
this way to allow one to change the setting in-game, but it breaks
slightly when you are running your train through itself. Running a
train through means that your network is broken and is thus a user
error which OpenTTD tries to graciously handle.

Fixing this bug means that we need to record whether this particular
setting was turned on or off at the time the reservation was made. This
means adding quite a bit of data to the savegame for solving an issue
that is basically an user error. We think it is not worth the effort.

### Duplicate (station) names after renaming [#3204]:

After renaming stations one can create duplicate station names. This
is done giving a station the same custom name as another station with
an automatically generated name.

The major part of this problem is that station names are translatable.
Meaning that a station is called e.g. '<TOWN> Central' in English and
'<TOWN> Centraal' in Dutch. This means that in network games the
renaming of a town could cause the rename to succeed on some clients
and fail at others. This creates an inconsistent game state that will
be seen as a 'desync'. Secondly the custom names are intended to fall
completely outside of the '<TOWN> <name>' naming of stations, so when
you rename a town all station names are updated accordingly.

As a result the decision has been made that all custom names are only
compared to the other custom names in the same class and not compared
to the automatically generated names.

### Extreme CPU usage/hangs when using SDL and PulseAudio [#3294], OpenTTD hangs/freezes when closing, OpenTTD is slow, OpenTTD uses a lot of CPU:

OpenTTD can be extremely slow/use a lot of CPU when the sound is
played via SDL and then through PulseAudio's ALSA wrapper. Under the
same configuration OpenTTD, or rather SDL, might hang when exiting
the game. This problem is seen most in Ubuntu 9.04 and higher.

This is because recent versions of the PulseAudio sound server
are configured to use timer-based audio scheduling rather than
interrupt-based audio scheduling. Configuring PulseAudio to force
use of interrupt-based scheduling may resolve sound problems for
some users. Under recent versions of Ubuntu Linux (9.04 and higher)
this can be accomplished by changing the following line in the
`/etc/pulse/default.pa` file:
        `load-module module-udev-detect`
to
        `load-module module-udev-detect tsched=0`

Note that PulseAudio must be restarted for changes to take effect. Older
versions of PulseAudio may use the module-hal-detect module instead.
Adding tsched=0 to the end of that line will have a similar effect.

Another possible solution is selecting the "pulse" backend of SDL
by either using `SDL_AUDIODRIVER=pulse openttd` at the command
prompt or installing the `libsdl1.2debian-pulseaudio` package from
Ubuntu's Universe repository. For other distributions a similar
package needs to be installed.

### OpenTTD not properly resizing with SDL on X [#3305]:

Under some X window managers OpenTTD's window does not properly
resize. You will either end up with a black bar at the right/bottom
side of the window or you cannot see the right/bottom of the window,
e.g. you cannot see the status bar. The problem is that OpenTTD does
not always receive a resize event from SDL making it impossible for
OpenTTD to know that the window was resized; sometimes moving the
window will solve the problem.

Window managers that are known to exhibit this behaviour are GNOME's
and KDE's. With the XFCE's and LXDE's window managers the resize
event is sent when the user releases the mouse.

### Incorrect colours, crashes upon exit, debug warnings and smears upon window resizing with SDL on macOS [#3447]:

Video handling with (lib)SDL under macOS is known to fail on some
versions of macOS with some hardware configurations. Some of
the problems happen only under some circumstances whereas others
are always present.

We suggest that the SDL video/sound backend is not used for OpenTTD
in combinations with macOS.

### Train crashes entering same junction from block and path signals [#3928]:

When a train has reserved a path from a path signal to a two way
block signal and the reservation passes a path signal through the
back another train can enter the reserved path (only) via that
same two way block signal.

The reason for this has to do with optimisation; to fix this issue
the signal update has to pass all path signals until it finds either
a train or a backwards facing signal. This is a very expensive task.

The (signal) setups that allow these crashes can furthermore be
considered incorrectly signalled; one extra safe waiting point for
the train entering from path signal just after the backwards facing
signal (from the path signal train) resolves the issue.

### Crashes when run in a VM using Parallels Desktop [#4003]:

When the Windows version of OpenTTD is executed in a VM under
Parallels Desktop a privileged instruction exception may be thrown.

As OpenTTD works natively on macOS as well as natively on Windows and
these native builds both don't exhibit this behaviour this crash is
most likely due to a bug in the virtual machine, something out of
the scope of OpenTTD. Most likely this is due to Parallels Desktop
lacking support for RDTSC calls. The problem can be avoided by using
other VM-software, Wine, or running natively on macOS.

### Entry- and exit signals are not dragged [#4378]:

Unlike all other signal types, the entry- and exit signals are not
dragged but instead normal signals are placed on subsequent track
sections. This is done on purpose as this is the usually more
convenient solution. There are little to no occasions where more
than one entry or exit signal in a row are useful. This is different
for all other signal types where several in a row can serve one
purpose or another.

### (Temporary) wrong colours when switching to full screen [#4511]:

On Windows it can happen that you temporarily see wrong colours
when switching to full screen OpenTTD, either by starting
OpenTTD in full screen mode, changing to full screen mode or by
ALT-TAB-ing into a full screen OpenTTD. This is caused by the
fact that OpenTTD, by default, uses 8bpp paletted output. The
wrong colours you are seeing is a temporary effect of the video
driver switching to 8bpp palette mode.

This issue can be worked around in two ways:
* Setting fullscreen_bpp to 32
* Setting up the 32bpp-anim or 32bpp-optimized blitter

### Can't run OpenTTD with the -d option from a MSYS console [#4587]:

The MSYS console does not allow OpenTTD to open an extra console for
debugging output. Compiling OpenTTD with the --enable-console
configure option prevents this issue and allows the -d option to use
the MSYS console for its output.

### Unreadable characters for non-latin locales [#4607]:

OpenTTD does not ship a non-latin font in its graphics files. As a
result OpenTTD needs to acquire the font from somewhere else. What
OpenTTD does is ask the operating system, or a system library, for
the best font for a given language if the currently loaded font
does not provide all characters of the chosen translation. This
means that OpenTTD has no influence over the quality of the chosen
font; it just does the best it can do.

If the text is unreadable there are several steps that you can take
to improve this. The first step is finding a good font and configure
this in the configuration file. See section 9.0 of README.md for
more information. You can also increase the font size to make the
characters bigger and possible better readable.

If the problem is with the clarity of the font you might want to
enable anti-aliasing by setting the small_aa/medium_aa/large_aa
settings to "true". However, anti-aliasing only works when a 32-bit
blitter has been selected, e.g. `blitter = "32bpp-anim"`, as with the
8 bits blitter there are not enough colours to properly perform the
anti-aliasing.

### Train does not crash with itself [#4635]:

When a train drives in a circle the front engine passes through
wagons of the same train without crashing. This is intentional.
Signals are only aware of tracks, they do not consider the train
length and whether there would be enough room for a train in some
circle it might drive on. Also the path a train might take is not
necessarily known when passing a signal.

Checking all circumstances would take a lot of additional
computational power for signals, which is not considered worth
the effort, as it does not add anything to gameplay.

Nevertheless trains shall not crash in normal operation, so making
a train not crash with itself is the best solution for everyone.

### Aircraft coming through wall in rotated airports [#4705]:

With rotated airports, specifically hangars, you will see that the
aircraft will show a part through the back wall of the hangar.

This can be solved by only drawing a part of the plane when being
at the back of the hangar, however then with transparency turned on
the aircraft would be shown partially which would be even weirder.

As such the current behaviour is deemed the least bad.
The same applies to overly long ships and their depots.

### Vehicles not keeping their "maximum" speed [#4815]:

Vehicles that have not enough power to reach and maintain their
advertised maximum speed might be constantly jumping between two
speeds. This is due to the fact that speed and its calculations
are done with integral numbers instead of floating point numbers.

As a result of this a vehicle will never reach its equilibrium
between the drag/friction and propulsion. So in effect it will be
in a vicious circle of speeding up and slowing down due to being
just at the other side of the equilibrium.

Not speeding up when near the equilibrium will cause the vehicle to
never come in the neighbourhood of the equilibrium and not slowing
down when near the equilibrium will cause the vehicle to never slow
down towards the equilibrium once it has come down a hill.

It is possible to calculate whether the equilibrium will be passed,
but then all acceleration calculations need to be done twice.

### Settings not saved when OpenTTD crashes [#4846]:

The settings are not saved when OpenTTD crashes for several reasons.
The most important is that the game state is broken and as such the
settings might contain invalid values, or the settings have not even
been loaded yet. This would cause invalid or totally wrong settings
to be written to the configuration file.

A solution to that would be saving the settings whenever one changes,
however due to the way the configuration file is saved this requires
a flush of the file to the disk and OpenTTD needs to wait till that
is finished. On some file system implementations this causes the
flush of all 'write-dirty' caches, which can be a significant amount
of data to be written. This can further be aggravated by spinning
down disks to conserve power, in which case this disk needs to be
spun up first. This means that many seconds may pass before the
configuration file is actually written, and all that time OpenTTD
will not be able to show any progress. Changing the way the
configuration file is saved is not an option as that leaves us more
vulnerable to corrupt configuration files.

Finally, crashes should not be happening. If they happen they should
be reported and fixed, so essentially fixing this is fixing the wrong
thing. If you really need the configuration changes to be saved,
and you need to run a version that crashes regularly, then you can
use the `saveconfig` command in the console to save the settings.

### Not all NewGRFs, AIs, game scripts are found [#4887]:

Under certain situations, where the path for the content within a
tar file is the same as other content on the file system or in another
tar file, it is possible that content is not found. A more thorough
explanation and solutions are described in section 4.4 of README.md.

### Mouse cursor going missing with SDL [#4997]:

Under certain circumstances SDL does not notify OpenTTD of changes with
respect to the mouse pointer, specifically whether the mouse pointer
is within the bounds of OpenTTD or not. For example, if you "Alt-Tab"
to another application the mouse cursor will still be shown in OpenTTD,
and when you move the mouse outside of the OpenTTD window so the cursor
gets hidden, open/move another application on top of the OpenTTD window
and then Alt-tab back into OpenTTD the cursor will not be shown.

We cannot fix this problem as SDL simply does not provide the required
information in these corner cases. This is a bug in SDL and as such
there is little that we can do about it.

### Trains might not stop at platforms that are currently being changed [#5553]:

If you add tiles to or remove tiles from a platform while a train is
approaching to stop at the same platform, that train can miss the place
where it's supposed to stop and pass the station without stopping.

This is caused by the fact that the train is considered to already
have stopped if it's beyond its assigned stopping location. We can't
let the train stop just anywhere in the station because then it would
never leave the station if you have the same station in the order
list multiple times in a row or if there is only one station
in theorder list (see #5684).

### Some houses and industries are not affected by transparency [#5817]:

Some of the default houses and industries (f.e. the iron ore mine) are
not affected by the transparency options. This is because the graphics
do not (completely) separate the ground from the building.

This is a bug of the original graphics, and unfortunately cannot be
fixed with OpenGFX for the sake of maintaining compatibility with
the original graphics.

### Involuntary cargo exchange with cargodist via neutral station [#6114]:

When two players serve a neutral station at an industry, a cross-company
chain for cargo flow can and will be established which can only be
interrupted if one of the players stops competing for the resources of
that industry. There is an easy fix for this: If you are loading at the
shared station make the order "no unload" and if you're unloading make
it "no load". Cargodist will then figure out that it should not create
such a route.

### Incorrect ending year displayed in end of game newspaper [#8625]

The ending year of the game is configurable, but the date displayed in
the newspaper at the end of the game is part of the graphics, not text.

So to fix this would involve fixing the graphics in every baseset,
including the original. Additionally, basesets are free to put this
text in different positions (which they do), making a proper solution
to this infinitely more complex for a part of the game that fewer than
1% of players ever see.
