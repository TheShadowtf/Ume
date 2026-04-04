# Ume Scripting Guide (Current Features + Roadmap)

This file documents what you can do right now with C# scripting in Ume, with many examples.

It also lists what should be added next to make Ume scripting good enough for full games.

## 1) Quick Start

1. Put scripts in your project script folder:
   - `Assets/Scripts/Source/*.cs`
2. Build scripts from editor:
   - `Script -> Build Scripts`
3. Add `Script` component to an entity.
4. Pick your class from the `Class` dropdown.
5. Press Play.

Example script template:

```csharp
using Ume;

namespace TestProject
{
    public class MyScript : Entity
    {
        public float Speed = 5.0f;

        void OnCreate()
        {
            // Called once at runtime start
        }

        void OnUpdate(float ts)
        {
            // Called every frame while running
        }
    }
}
```

## 2) What Works Right Now

## 2.0 Scene Hierarchy Status

- True parent-child grouping (nesting entities like Unity hierarchy) is not implemented yet.
- Current hierarchy is effectively flat.
- Drag-drop currently works for assigning entity references, not for reparenting transform hierarchy.

## 2.1 Entity API

Available on all scripts inheriting `Entity`:

- `ID` (ulong)
- `Tag` (string get/set)
- `Translation` (Vector3 get/set)
- `OnGround` / `IsOnGround` (bool)
- `HasComponent<T>()`
- `GetComponent<T>()`
- `FindEntityByName(string)`
- `FindEntityByTag(string)`
- `CompareTag(string)`
- `As<T>()`

Example:

```csharp
using Ume;

namespace TestProject
{
    public class TagExample : Entity
    {
        void OnUpdate(float ts)
        {
            if (CompareTag("Player"))
            {
                // do player logic
            }

            Entity ground = FindEntityByTag("Ground");
            if (ground != null)
            {
                // found at least one entity with tag Ground
            }
        }
    }
}
```

Note:
- `Tag` currently maps to the same value shown as entity name in Scene Hierarchy.

## 2.2 Components You Can Access in C#

### TransformComponent

- `Translation`
- `Rotation`
- `Scale`

```csharp
var tr = GetComponent<TransformComponent>();
tr.Translation = new Vector3(2.0f, 1.0f, tr.Translation.Z);
```

### Rigidbody2D

- `Type` (Static/Dynamic/Kinematic)
- `LinearVelocity` (read)
- `ApplyLinearImpulse(Vector2 impulse, bool wake)`
- `ApplyLinearImpulse(Vector2 impulse, Vector2 worldPoint, bool wake)`

```csharp
var rb = GetComponent<Rigidbody2D>();
rb.ApplyLinearImpulse(new Vector2(2.0f, 0.0f), true);
```

### TextComponent

- `Text`
- `Color` (Vector4)
- `Kerning`
- `LineSpacing`

```csharp
var text = GetComponent<TextComponent>();
if (text != null)
{
    text.Text = "Coins: 12";
    text.Color = new Vector4(1, 1, 0, 1);
}
```

## 2.3 Input

- `Input.IsKeyDown(KeyCode.X)`

```csharp
if (Input.IsKeyDown(KeyCode.Space) && IsOnGround)
{
    // jump
}
```

## 2.4 Screen Size

- `Screen.Width`
- `Screen.Height`

```csharp
// Example: keep entity inside game viewport pixel bounds
Vector3 p = Translation;
p.X = Mathf.Clamp(p.X, 0.0f, Screen.Width);
p.Y = Mathf.Clamp(p.Y, 0.0f, Screen.Height);
Translation = p;
```

## 2.5 Ground Detection (Unity-like setup)

Ground check now uses contact with entities that have `Rigidbody 2D -> Is Ground` enabled.

Setup:

1. Ground entities:
   - Add `Rigidbody 2D`
   - Set body type to `Static`
   - Enable `Is Ground`
   - Add collider component
2. Player entity:
   - Add `Rigidbody 2D` (Dynamic)
   - Add collider

Script usage:

```csharp
if (IsOnGround)
{
    // grounded movement / jump logic
}
```

## 2.6 Script Inspector Fields (with examples)

When fields are public, they appear in Script component properties.

Supported inspector field types now:

- `float`
- `double`
- `bool`
- `int`
- `string`
- `Vector2`
- `Vector3`
- `ColorPicker` (color widget)
- `Entity` (entity reference picker + drag drop from Scene Hierarchy)

Example:

```csharp
using Ume;

namespace TestProject
{
    public class FollowTarget : Entity
    {
        public Entity Target;
        public Vector3 Offset = new Vector3(0, 2, 0);
        public float Smooth = 6.0f;
        public bool LockZ = true;

        void OnUpdate(float ts)
        {
            if (Target == null)
                return;

            Vector3 desired = Target.Translation + Offset;
            Vector3 current = Translation;
            Translation = Vector3.Lerp(current, desired, Mathf.Clamp01(Smooth * ts));

            if (LockZ)
                Translation = new Vector3(Translation.X, Translation.Y, current.Z);
        }
    }
}
```

Entity drag-drop flow:

1. Select entity with script.
2. In Script field `Target`, drag another entity from Scene Hierarchy and drop it.

## 2.7 Math Types

Usable now:

- `Vector2`, `Vector3`, `Vector4`
- `Quaternion`
- `Mathf`

Typical helpers:

- `normalized`, `magnitude`, `sqrMagnitude`
- `Lerp`, `MoveTowards`, `Dot`, `Cross`
- `Mathf.Clamp`, `Mathf.Clamp01`, `Mathf.Lerp`, trig functions

Example:

```csharp
Vector2 input = new Vector2(1, 1).normalized;
float speed = Mathf.Lerp(2.0f, 8.0f, 0.5f);
```

## 3) Practical Player Example (Current API)

```csharp
using Ume;

namespace TestProject
{
    public class PlayerController : Entity
    {
        public float MoveImpulse = 2.5f;
        public float JumpImpulse = 7.5f;

        private Rigidbody2D _rb;

        void OnCreate()
        {
            _rb = GetComponent<Rigidbody2D>();
        }

        void OnUpdate(float ts)
        {
            if (_rb == null)
                return;

            float horizontal = 0.0f;
            if (Input.IsKeyDown(KeyCode.A)) horizontal -= 1.0f;
            if (Input.IsKeyDown(KeyCode.D)) horizontal += 1.0f;

            if (horizontal != 0.0f)
                _rb.ApplyLinearImpulse(new Vector2(horizontal * MoveImpulse, 0.0f), true);

            if (Input.IsKeyDown(KeyCode.Space) && IsOnGround)
                _rb.ApplyLinearImpulse(new Vector2(0.0f, JumpImpulse), true);
        }
    }
}
```

## 4) Known Limits Right Now

These are important so you know where scripting can still block bigger games.

- No delta time API in `Time` yet (`Time` class is empty).
- No collision callbacks in scripts (`OnCollisionEnter2D`, etc.).
- No trigger callbacks in scripts.
- No direct force API (`AddForce`) mode variants.
- No Rigidbody2D velocity setter (read-only currently).
- No prefab system in scripting.
- No scene loading API from script.
- No audio API in script layer.
- No animation API/controller in script layer.
- No coroutine/timer utility in script layer.
- No event bus/signals in scripting.
- No layer/collision matrix system yet.
- Tag system exists, but dedicated Layer workflow is not there yet.
- Inspector attributes like `[Range]`, `[Header]`, `[Tooltip]` not implemented.
- No asset reference fields (textures/audio/prefabs) in script inspector yet.
- No script debugging UX beyond logs.

## 5) What To Add Next (Priority Roadmap)

If your goal is "good enough to build full games", this is a strong order.

## Phase A (Core Gameplay)

1. Add `Time.deltaTime`, fixed step timing, unscaled time.
2. Add `Rigidbody2D` quality-of-life:
   - velocity setter
   - AddForce / ForceMode2D
   - gravityScale / drag
3. Add 2D collision callbacks to C#:
   - `OnCollisionEnter2D`, `OnCollisionStay2D`, `OnCollisionExit2D`
   - trigger equivalents
4. Add Layer + collision matrix in physics and inspector.

## Phase B (Authoring + Inspector)

1. Inspector attributes:
   - `[Header]`, `[Space]`, `[Tooltip]`, `[Range]`, `[Min]`
2. Better object references:
   - Component references
   - Asset references (textures, audio, prefabs)
3. Better script compile errors in inspector panel with file and line links.

## Phase C (Game Systems)

1. Scene management API:
   - load scene
   - reload scene
   - spawn APIs
2. Prefab workflow (instantiate and save overrides).
3. Audio API (play one-shot, loop, 2D/3D params).
4. Animation state API.
5. Save/load utility (JSON or binary helpers).

## Phase D (Scale + Polish)

1. Coroutines or async gameplay helpers.
2. Script events/signals.
3. Better hot-reload safety for script fields and references.
4. Script profiler hooks and runtime diagnostics.

## 6) Troubleshooting Checklist

If script compiles but behavior looks dead:

1. Confirm script class is selected in Script component.
2. Confirm scene is in Play mode (not Edit).
3. Confirm script project built with no errors.
4. Confirm Player has required components (Rigidbody2D + collider).
5. Confirm movement values are not tiny (impulse/speed too low).
6. Click game viewport once so input focus is there.
7. Save scene after assigning script fields.

## 7) Suggested "First Game" Minimal Stack

Good baseline for a first platformer in current Ume:

1. Player with Rigidbody2D + BoxCollider2D + PlayerController script.
2. Ground with Static Rigidbody2D + collider + `Is Ground = true`.
3. Camera script with public `Entity Target` assigned via inspector.
4. Text UI entity for score/time.
5. Tag usage (`Player`, `Ground`, `Hazard`) for lightweight game rules.

---

If you want, next step I can generate a second file with a small "Starter Script Pack" (PlayerController, CameraFollow, CoinPickup, DamageZone, WinTrigger) ready to drop in your project.
