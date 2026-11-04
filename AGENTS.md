# No computer use

Never use computer-use tools, desktop automation, UI automation, or automated
screenshots. Use shell commands, source inspection, build tools, and logs for
launching and verifying the game. Leave visual and audio checks to the user.

# Diagnostic code after testing

After running tests for an investigation or fix, always explicitly ask whether
diagnostic code inside the actual game source needs cleanup before calling the
work complete. Inspect the source first so the question identifies any remaining
diagnostic code concretely.

This includes temporary startup checks, synthetic fixtures, test-only callbacks,
environment switches, capture hooks, traces, and instrumentation added to game
code. A compile-time guard does not make leftover diagnostic code acceptable.
Standalone tests outside the game source are a separate concern.

If the user has already requested diagnostic cleanup for the current task, perform
that cleanup without asking again. Preserve the framerate profiler unless the
user requests its removal.

# Code formatting

Always prettify code you write or modify before handing the work back. Follow
the surrounding style and use the repository's formatter configuration. Run
clang-format with `src/.clang-format` on changed C/C++ sources, headers, include
files, and other supported files. Keep scripts readable and consistently
formatted too; do not leave compressed one-line code or awkward generated
formatting. Review the formatted diff before declaring the work complete.

# README scope

Do not modify README.md unless the user explicitly asks for a README change.
Code changes, releases, testing, cleanup, and tool reorganization do not imply
permission to edit README.md. Report development and verification details in
chat instead.

When the user explicitly requests a README change, keep it concise and focused
on information useful to players. Do not add internal implementation details,
test commands, test locations, validation results, or developer tooling notes
unless the user explicitly requests those details.

# Tools folder scope

Keep `tools/` limited to player-facing build tools, level extraction/export
tools, and the level unlocker. Do not put testing, verification, diagnostic,
research, or internal release-management scripts or fixtures in this folder.
Keep those in the git-ignored `harness_tools/` folder instead.

Required production build configuration, including `game_compile_context.json`,
may live alongside the build scripts in `tools/`. Diagnostic or test metadata
belongs in `harness_tools/` or another git-ignored investigation/output folder.
Test-only headers and include fragments also belong in `harness_tools/`, not
`src/` or `tools/`.
