MAX_SPEED = 10

function init()
    robot.leds.set_all_colors("red")
    robot.state = "foraging"

    -- WARNING: Do not write to these two variables!
    robot.has_food = false
    robot.food_value = 0
end

function step()

    -- Draw debug message above this robot
    robot.message = robot.id .. ": " .. robot.state .. " - " .. robot.food_value

    -- Calculate avoidance and light vectors
    local avoidance_vector = getAvoidanceVector()
    local light_vector = getLightVector()

    -- Foraging finite state machine
    if robot.state == "foraging" then
        foraging()
    elseif robot.state == "returning" then
        returning()
    end

end

function reset()
    init()
end

function destroy()

end

function foraging()

    -- If the robot has picked up a food item
    if robot.has_food then
        -- Set LEDs to green, and transition to returning state
        robot.leds.set_all_colors("green")
        robot.state = "returning"
    -- If the robot doesn't have a food item
    else
        -- If the robot is in the nest
        if inNest() then
            -- Perform anti-phototaxis, while avoiding obstacles
            setWheelSpeedsFromVector(subtractVectors(getAvoidanceVector(), getLightVector()))
        -- If the robot is outside the nest
        else
            -- Just avoid obstacles
            setWheelSpeedsFromVector(getAvoidanceVector())
        end
    end

end

function returning()

    -- If the robot has deposited its food item at the nest
    if not robot.has_food then
        -- Set LEDs to red, and transition to foraging state
        robot.leds.set_all_colors("red")
        robot.state = "foraging"
    -- If the robot has a food item
    else
        -- Perform phototaxis, while avoiding obstacles
        setWheelSpeedsFromVector(addVectors(getAvoidanceVector(), getLightVector()))
    end

end

function getAvoidanceVector()

    local avoidance_vector = { x = 0, y = 0 }

    -- Calculate vector to obstacles (if any)
    for i = 1, #robot.proximity do
        avoidance_vector = addVectors(avoidance_vector, newVectorFromPolarCoordinates(robot.proximity[i].value, robot.proximity[i].angle));
    end

    -- If there are no obstacles straight ahead
    if (getVectorAngleDegrees(avoidance_vector) > -5 and getVectorAngleDegrees(avoidance_vector) < 5) and getVectorLength(avoidance_vector) < 0.1 then
        -- Return a unit vector along the x-axis (straight ahead)
        return newVectorFromPolarCoordinates(1, 0)
    else
        -- Otherwise, return a unit vector 180 degrees away from the obstacle
        avoidance_vector = normaliseVectorLength(avoidance_vector)
        return { x = -avoidance_vector.x, y = -avoidance_vector.y }
    end

end

function getLightVector()

    local light_vector = { x = 0, y = 0 }

    -- Calculate vector to lights
    for i = 1, #robot.light do
        light_vector = addVectors(light_vector, newVectorFromPolarCoordinates(robot.light[i].value, robot.light[i].angle));
    end

    -- If any light was detected
    if getVectorLength(light_vector) > 0 then
        -- Return a unit vector towards the light source
        return newVectorFromPolarCoordinates(1, getVectorAngleRadians(light_vector))
    else
        -- Otherwise, return a zero vector
        return { x = 0, y = 0 }
    end

end

function setWheelSpeedsFromVector(vector)

    -- robot.vectors = { { x = normaliseVectorLength(vector).x, y = normaliseVectorLength(vector).y, color = "yellow" } }

    -- Normalise angle of vector
    local heading_angle = normaliseAngle(getVectorAngleDegrees(vector))

    left_speed = MAX_SPEED
    right_speed = MAX_SPEED

    -- Turn left or right, based on angle of vector
    if heading_angle > 10 then
        left_speed = 0
    elseif heading_angle < -10 then
        right_speed = 0
    end

    -- Set wheel speeds
    robot.wheels.set_velocity(left_speed, right_speed)

end

function inNest()

    -- If the two rear motor ground sensors detect grey, then the robot is completely within the nest
    if robot.motor_ground[2].value > 0.25 and
       robot.motor_ground[2].value < 0.75 and
       robot.motor_ground[3].value > 0.25 and
       robot.motor_ground[3].value < 0.75 then

        return true
    else
        return false
    end

end

-- Utility functions
function newVectorFromPolarCoordinates(length, angle_radians)
    return { x = (length * math.cos(angle_radians)),
             y = (length * math.sin(angle_radians)) }
end

function addVectors(a, b)
    return { x = a.x + b.x,
             y = a.y + b.y }
end

function subtractVectors(a, b)
    return { x = a.x - b.x,
             y = a.y - b.y }
end

function getVectorAngleRadians(vector)
    return math.atan2(vector.y, vector.x)
end

function getVectorAngleDegrees(vector)
    return math.deg(getVectorAngleRadians(vector))
end

function getVectorLength(vector)
    return math.sqrt((vector.x * vector.x) + (vector.y * vector.y))
end

function normaliseVectorLength(vector)
    local length = getVectorLength(vector)
    return { x = vector.x / length,
             y = vector.y / length}
end

function normaliseAngle(angle_degrees)
    while angle_degrees > 180 do
        angle_degrees = angle_degrees - 360
    end

    while angle_degrees < -180 do
        angle_degrees = angle_degrees + 360
    end

    return angle_degrees
end