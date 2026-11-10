# How to play

Choose **Game**, a vehicle, then training or an unlocked episode. Follow the incoming messages and radar to find your objectives.

The keyboard and mouse controls below are the default **Standard** profile bindings. Change them under **Setup → Input configuration → Edit**; a custom save may use different keys. Mouse steering is disabled in this restoration, but mouse buttons work. Gamepad controls work alongside the selected keyboard profile.

## Flying and fighting

| Control | Action |
|---|---|
| A / Z | Accelerate / brake |
| Left / Right arrows | Turn left / right |
| Down / Up arrows | Pitch up / down (inverted flight controls) |
| Ctrl or left mouse button | Fire primary weapon |
| Space or right mouse button | Fire secondary weapon |
| Tab | Review previous incoming messages |
| Esc | Ask to abandon the mission; pauses single-player |
| Y / N | Confirm abandonment / resume; Esc also cancels the prompt |

## Weapon selection

Weapons must be collected/unlocked and have usable ammunition.

| Key | Weapon | Key | Weapon |
|---|---|---|---|
| 1 | Ion gun | 6 | Homie missile |
| 2 | Lead buster | 7 | Sonic blast |
| 3 | Magnetic plasma | 8 | TNT mine |
| 4 | Nitro crackers | 9 | Fat lady |
| 5 | Stun ray | 0 | Static coating |

## Camera

| Key | View/action |
|---|---|
| F1 | Cockpit |
| F2 | Tail view |
| F3 | Trace camera |
| F4 | Fly-by camera |
| F5 | Circle camera |
| F6 | Side view |
| Numpad 4 / 6 | Look left / right |
| Numpad 2 / 8 | Look up / down |

## Gamepad

Connect a supported gamepad before starting or while the game is running. These gamepad bindings are fixed; the input configuration menu edits the keyboard profile.

| Control | Action |
|---|---|
| Left stick left / right | Turn left / right |
| Left stick down / up | Pitch up / down (inverted flight controls) |
| Right stick | Aim / look around |
| Right trigger | Accelerate with analogue thrust |
| Left trigger | Brake / reverse with analogue thrust |
| Left bumper (LB / L1) | Fire primary weapon |
| Right bumper (RB / R1) | Fire secondary weapon |
| D-pad left | Cycle primary weapons |
| D-pad right | Cycle secondary weapons |
| Select / Back / Share | Cycle camera views |
| Start / Menu / Options | Ask to abandon the mission; pauses single-player. Press again to resume. |
| Bottom face button (A / Cross) | Confirm abandonment |
| Right face button (B / Circle) | Cancel abandonment / resume |

Stick and trigger dead zones help prevent drift. Keyboard controls remain active while using a gamepad. If the pad disconnects, its thrust, aiming and firing inputs are released automatically, so you can continue on the keyboard. Reconnecting restores gamepad control automatically.

## Menus

Use **Up/Down** to select, **Enter/Space/left mouse** to accept, and **Esc/right mouse** to go back.

On a gamepad, use **D-pad Up/Down** or the **left stick** to select, the **bottom face button (A / Cross)** to accept, and the **right face button (B / Circle)** to go back.

## Multiplayer

The game uses UDP port **27890** by default. On the same local network, join using the host computer's local IPv4 address. The host's firewall must allow incoming UDP on the chosen port.

### From the Network menu

1. Both players select **Network** from the main menu and choose a vehicle.
2. The host selects **Host** and shares their IPv4 address.
3. The joining player selects **Join by IP**, enters the host's IPv4 address, then presses **Enter** or **A / Cross** to connect.

### From the command line

Run the host command on one computer and the join command on the other. Replace `192.168.1.50` with the host's IPv4 address.

On Windows, run these from the game folder.

```powershell
.\BHunter-source.exe --host
.\BHunter-source.exe --join 192.168.1.50
```

On Linux, run these from the game folder. The library path lets the packaged build find its bundled SDL.

```sh
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./BHunter-source --host
./BHunter-source --join 192.168.1.50
```

To use a different UDP port, add the same `--port` value to both commands, such as `--port 27900`. The in-game menu uses the default port.

Set `DefaultVehicle` under `[Multiplayer]` in `game-config.ini`. Use `0` for Barletta (the default), `1` for Stinger, or `2` for Viper. Add `--vehicle` with a value to either command to override the INI for that launch.

### Custom levels

Put the `.bhlevel` package in `GAME_DATA` beside `Bhunter.bin`. Pass the package name without its extension. Names can contain up to 48 ASCII letters, digits, hyphens, and underscores.

```powershell
.\BHunter-source.exe --level crossroads
.\BHunter-source.exe --host --level crossroads
.\BHunter-source.exe --join 192.168.1.50
```

The first command starts solo flight. The second hosts the arena. Every player needs the same package file in their own `GAME_DATA` folder. The host selects the level for everyone, including players who join through the Network menu. On Linux, use the same arguments with `./BHunter-source`.

From a source checkout, run `python tools/build_test_arena.py` to generate `build/GAME_DATA/crossroads.bhlevel`. It includes the arena geometry, scripts, and five custom road textures. The remaining materials come from your installed `Bhunter.bin`.

To package another level, put its placement file at `level.lev` and add its custom assets under `bsp/`, `textures/`, and `sound/` as needed. Include a `bsp/blocks.txt` for a custom block library and define starts with the block scripts. You can include a `level.pth` navigation cache. Run `python tools/pack_level.py path/to/level build/GAME_DATA/my-level.bhlevel` and share the resulting file.

## Original single-player cheats

Type these during gameplay, without pressing Enter:

| Sequence | Effect |
|---|---|
| IAKFA | Unlock weapons and replenish ammunition |
| IADQD | Toggle god mode |
| IABAT | Switch to the alternate vehicle model |
