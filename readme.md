# Distance, Time-of-Impact, and Closest-Point-of-Approach between Convex 2D Shapes

![CPA vs VO](/img/cover.png)
*Closest-point-of-approach has relation to velocity obstacle, but it has a gradient instead of just boolean result.*

## Overview

This repository contains example code to calculate distance, time-of-impact, and closest-point-of-approach between all combinations of circle, pill (capsule), and rounded rectangle.

The circle-vs-circle and circle-vs-pill are calculated analytically, and the rest is handled combining the same calculations with partial Minkowski sum. The algorithm can be extended to handle convex polygons with radius.

Compared to GJK, the algorithm is about 2-8x faster depending on shape and configuration (take it with grain of salt). GJK has higher initial cost, but scales more slowly with complexity and can handle just about any shape.

The code is made with crowd simulation in mind, but can be useful for continuous collision detection too.

## Nearest Distances and Minkowski sum

The nearest distance and separation direction between two circles is easy to calculate. You just calculate the direction between the centers of the circles. The direction is the separation direction, and the distance minus radius of the both circles is how much the circles penetrate.

Distance between a circle and pill is simple too, first you project the circle's location on the stem of the pill, and then calculate direction and distance between the projected point and the circle taking both radii into account.

The other combinations can be expressed with circle and segment distances, but it starts to get verbose. Instead we use a common solution for all of them that uses Minkowski sum. Minkowski sum changes the calculations to be between a point and a shape that is combination of the both shapes.

![Shapes](/img/sums.svg)

*Shapes and their Minkowski sums.*


A pill is essentially a Minkowski sum of a segment and circle, or rounded rect is sum of circle and rectangle. So by calculating the Minkowski sum of the "stick" parts of the shapes, and then dealing the radius by just summing them and reducing it from the distance to the skeleton shape, we have our solution.

Calculating the sum of shapes made from segments is pretty simple and can be calculatd in linear time. Two segments becomes a parallelogram, and two rectangles make an (likely unregular) octagon. This allows us to use the simple projection and distance between circles to deal with all the shapes.

But how does one calculate the minkowski sum of the segments? For two convex polygonal shapes, we start by making sure the first vertex is oriented the same way. A classic way for any convex polygon is to find the lowest left point (in case the bottom is flat). Then build the sum by adding an edge from the polygon whose edge points more left (can be detected using a 2D cross product), and iterate until all edges from both polygons are added. The edges are added relative to the last position. The result is new convex polygon that is Minkowski sum of the both conex polygons. The result is the same as if we swept one shape around the perimeter of another, and took the outline.

![Partial sum](/img/partialsum.svg)

*Partial Minkowski sum of segment and rect. Only the edges that are facing the relative direction are needed to calculate the distance.*


We make a little optimization to the classic method by only considering the edges of the polygons that are facing towards the direction between the shapes, and combine them into a partial Minkowski sum. This cuts the computation in half in next step.

When calculate the distance, we take the partial Minkowski sum shape is created around (0,0), and proceed to find the distance between the chain and the relative position between the shapes. (Side note: I think if the test position was at (0,0), and the sum shape would be offset by the relative position, it would be a Minkowski difference). First the nearest vertex is found, and then we continue to check if either of the segments connecting to the nearest vertex contains a closer result. Finally radius of the both shapes are taken into account when reporting the distance.


## Calculating Time-of-Impact (TOI)

Time of impact takes two shapes, and two velocities and finds the time in future where they collide. Using the Minkowski sum, most raycasting codes can be turned in to TOI calculation by making the ray origin be the relative position between the shapes, and casting the ray at the direction of the relative velocity. The shape to cast gainst should be Minkowsku sum of the two shapes.

![Relative velocity](/img/relvel.svg)

*Relative velocity and Minkowskis sum can be used to turn TOI calculation into raycasting problem.*


The simplest to calculate is TOI between two moving circles. The Minkowski sum of two circles is a circle with radius of both circles summed. Similarly, circle-segment or circle-pill (capsule) TOI can be calculated using ray-capsule intersection and summing the radii.

For more complicated shapes we can caculate the Minkowski sum of the segments of the two shapes, and then raycast againts that, taking the radius into account similarly a we did for circle-segment case above.

Similarly to distance calculations, we can optimize the summed shape by just handling only the segments that are facing the ray direction. By calculating the chain in relation to the ray direction we can also use the first and last point on the chain to quickly see if the whole shape is missed.


## Closest-Point-of-Approach (CPA), or time, actually

Closes point-of-approach is either at the time-of-impact of the two moving shapes, or the time the shapes are the closest to each other. For the case of colliding shapes, we use the method of calculating TOI from above. To simplify the nearest point of two moving objects problem, we use relative velocity too.

![Closest point of approach of circles](/img/cpasimple.svg)

*Closest point of approach when there is no imminent collision. The approach time is calculated by projecting the nearest point of the Minkowski sum on the relative velocity. For circles the projected center is the same as the projected nearest point.*


For two moving circles, the closest point of approach (sans the colliding case) can be calculated by using their relative velocity and making the second shape stationary. In that case the CPA is the second circle's center projected on the relative velocity. For more complex shapes, we should project the point of the shape that is nearest to the relative velocity.

There's a neat trick to turn a circle-circle TOI into CPA. When there's no collision between the moving circles, the root finding fails, and no hit is reported. If we instead clamp the discriminant to 0, the function just projects the circle's point on the line and the result is CPA in all situations.

```C++
bool circleCircleTOI(const Vec2 pos, const Vec2 vel, const float rad, const Vec2 center, float& t)
{
	const Vec2 relPos = pos - center;
    const float a = dot(vel, vel);
	const float b = dot(vel, relPos);
	const float c = dot(relPos, relPos) - rad * rad;
	const float disc = b*b - a * c;
    if (disc < 0.0f) return false; // no hit
	t = (-b - sqrtf(disc)) / a;
	return true;
}
```

```C++
bool circleCircleCPA(const Vec2 pos, const Vec2 vel, const float rad, const Vec2 center, float& t)
{
	const Vec2 relPos = pos - center;
    const float a = dot(vel, vel);
	const float b = dot(vel, relPos);
	const float c = dot(relPos, relPos) - rad * rad;
	const float disc = b*b - a * c;
	t = (-b - sqrtf(maxf(0.0f, disc))) / a;
	return disc > 0.0f;
}
```

The projected nearest point between circle and ray/segment is the same as the projected circle center. We can use that into our advantage to calculate the nearest point to the more complex shapes.

![Closest point of approach](/img/cpa.svg)

*For TOI and CPA calculations the partial sum is calculated along the relative velocity. The extrema points can be used to detect if the ray will miss the shape completely, and to calculate the closest point of approach.*


The circle-capsule TOI code uses circle-circle test for testing the caps, so it can be translated to CPA too (the body will report a hit). For the more complicated cases we go back to the partial Minkowski sum. As mentioned earlier, we can easily detect when we are not going to hit the Minkowski chain by observing the first and last point on the chain. These extrema points just so happen to be the points that we are interested when projecting the nearest point of the shape on the relative velocity to find the closes-point-of-approach.

## The Code

The example code implements nearest-distance and closet-point-of-approach functions, which have special handling for circle-circle, and circle-segment, and the rest is handled with the Minkowski sum chain method. The CPA result is tagged as `hit` when the closest-point-of-approach leads to collision.

There area a couple of visual toys in the test.cpp to explore the code.

Related links:
- *Wikipedia in Minskowski sum:* https://en.wikipedia.org/wiki/Minkowski_addition
- *Minkowski sum of convex polygons:*  https://cp-algorithms.com/geometry/minkowski.html
- *GJK/CA based TOI and distance:* https://github.com/RandyGaul/cute_headers/blob/master/cute_c2.h

## License
zlib