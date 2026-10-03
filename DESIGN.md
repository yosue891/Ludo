# Ludo Live — Design System

## Direction

Ludo Live should feel like a premium native game app, not a dashboard. The shell is dark and quiet so the physical-board colors remain the focus. Surfaces use soft depth, restrained borders and large radii. The board is the hero.

## Tokens

- Canvas: `#0d0f12`
- Surface: `#15181d`
- Elevated: `#1b1f25`
- Text: `#f5f2e9`
- Muted: `#9ca3ad`
- Border: `rgba(255,255,255,.08)`
- Red: `#ef5b5b`
- Green: `#2bc875`
- Yellow: `#f6c945`
- Blue: `#4b8df8`
- Radius: 18 / 24 / 32px
- Motion: 160–260ms, ease-out; disable nonessential motion for reduced-motion users

## Interaction

All actions use semantic buttons with visible focus and at least 44px touch targets. The current player, selectable tokens and connection state must not rely on color alone. Layout transitions must not move the board unexpectedly.

## Responsive

Desktop uses a two-column game layout: board plus control rail. Mobile stacks board over a sticky-ish control surface with the board kept square and fully visible.
