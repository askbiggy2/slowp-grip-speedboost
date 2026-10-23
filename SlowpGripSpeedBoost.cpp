using UnityEngine;
using UnityEngine.XR;

// Hold right grip to fly: gravity is cancelled and your current speed gets a small boost.
[RequireComponent(typeof(Rigidbody))]
public class GripFlyBoost : MonoBehaviour
{
    [Tooltip("Speed to reach, relative to your speed when grip was pressed (1.25 = 25% faster).")]
    public float boostMultiplier = 1.25f;

    [Tooltip("How quickly the boost kicks in (m/s per second).")]
    public float boostAcceleration = 8f;

    [Tooltip("The boost never pushes you above this speed (m/s).")]
    public float maxSpeed = 12f;

    [Tooltip("1 = no falling while holding grip, 0 = normal gravity.")]
    [Range(0f, 1f)] public float antiGravity = 1f;

    [Tooltip("Below this speed (m/s) there is no direction to boost in.")]
    public float minSpeed = 0.2f;

    Rigidbody rb;
    InputDevice rightHand;
    bool wasHeld;
    float targetSpeed;

    Vector3 Velocity
    {
#if UNITY_6000_0_OR_NEWER
        get => rb.linearVelocity;
        set => rb.linearVelocity = value;
#else
        get => rb.velocity;
        set => rb.velocity = value;
#endif
    }

    void Awake() => rb = GetComponent<Rigidbody>();

    void FixedUpdate()
    {
        if (!rightHand.isValid)
            rightHand = InputDevices.GetDeviceAtXRNode(XRNode.RightHand);

        rightHand.TryGetFeatureValue(CommonUsages.gripButton, out bool held);

        Vector3 velocity = Velocity;
        float speed = velocity.magnitude;

        // Lock in the boosted speed at the moment grip is pressed
        if (held && !wasHeld)
            targetSpeed = Mathf.Min(speed * boostMultiplier, maxSpeed);
        wasHeld = held;

        if (!held) return;

        if (rb.useGravity)
            rb.AddForce(-Physics.gravity * antiGravity, ForceMode.Acceleration);

        if (speed > minSpeed && speed < targetSpeed)
        {
            float newSpeed = Mathf.MoveTowards(speed, targetSpeed, boostAcceleration * Time.fixedDeltaTime);
            Velocity = velocity / speed * newSpeed;
        }
    }
}
