# Slowp 0.99.144 reverse-engineering notes

Based on the supplied `dump.cs` metadata:

## Input state
- `Slowp.Input.XRCommonBoolInput.Update()` RVA `0x1BF44EC`.
- Fields: `Node` offset `0x30`, `Usage` offset `0x34`, `Value` offset `0x40`.
- `UnityEngine.XR.XRNode.RightHand = 5`.
- `Slowp.Input.XRCommonUsage.GripButton = 5`.
- `Slowp.Input.XRCommonUsage.Grip = 14` is the analog grip axis, not the boolean grip button.

The prototype hooks `XRCommonBoolInput.Update` after the original method and only samples objects where Node is RightHand and Usage is GripButton.

## Movement path
- `Slowp.Player.Board` TypeDefIndex 4458.
- `Board.Force(...)` RVA `0x1CFC61C` (14 explicit parameters).
- Signature in the metadata dump returns `Vector3`; it receives `ref float forwardCoeff`.
- Prototype passes a temporary `forwardCoeff * 1.25` only while the right grip state is true, avoiding persistent changes to game data.

## Remaining verification
- Confirm runtime `MethodInfo::methodPointer` layout and method resolution work with this Unity/IL2CPP build.
- Confirm QuestLoader calls `init()` and `load()` at the expected time for this modloader version.
- Confirm Dobby is available and compatible.
- Confirm the C++ ABI of `Vector3` return / ref parameters matches the ARM64 IL2CPP call.
- Confirm in-game whether forward coefficient scaling produces the intended movement effect; gravity is not modified by this implementation.
