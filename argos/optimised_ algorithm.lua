--ID =Y3864254
MAX_SPEED = 10
max_explore_step = 1500

function init()
	robot.leds.set_all_colors("blue")
	robot.state = "exploring"
	robot.range_and_bearing.set_data(1,10)
	robot.range_and_bearing.set_data(2,0)
	best_value_have_known = 0
	current_step = 0
	explore_step = 0
    -- WARNING: Do not write to these two variables!
    robot.has_food = false
    robot.food_value = 0
end

function step()

	current_step = current_step + 1 
    -- Draw debug message above this robot
    robot.message = robot.id .. ": " .. robot.state .. " - " .. robot.food_value

    -- Calculate avoidance and light vectors
    local avoidance_vector = getAvoidanceVector()
    local light_vector = getLightVector()
	local diatance_value = calculate_value_to_nest()

    -- Foraging finite state machine
    if robot.state == "exploring" then
        exploring()
    elseif robot.state == "foraging" then
        foraging()
    elseif robot.state == "returning" then
        returning()
    elseif robot.state == "marking" then
        marking()
    end

end

function reset()
    init()
end

function destroy()
robot.colored_blob_omnidirectional_camera.disable()

end

function foraging()
    local food_vector1 = getFoodVector()
	local diatance_value = calculate_value_to_nest()
    local leader_flag = 1
    -- If the robot has picked up a food item
    if robot.has_food then
   	--Determine if it is a leader
 	    for i = 1, #robot.range_and_bearing do
		if robot.range_and_bearing[i].range < 70 then
			if robot.range_and_bearing[i].data[1] == 1 then --if there are leader nearby
				leader_flag = 0 
			end
		end
		if 10*diatance_value*robot.food_value <= best_value_have_known then 
		--see which one is better
			leader_flag = 0 
		end
		
	   end
	   if leader_flag == 1 then--if it is a leader robot
		  robot.leds.set_all_colors("red")
		  robot.state = "marking"
		  robot.range_and_bearing.set_data(1,1)
		
			best_value_have_known = 10*diatance_value*robot.food_value
		  robot.range_and_bearing.set_data(2,best_value_have_known)

	  else
		  robot.state = "returning"
	   end
    -- If the robot doesn't have a food item
    else
        -- If the robot is in the nest
        if inNest() then
            -- Perform anti-phototaxis, while avoiding obstacles
            setWheelSpeedsFromVector(subtractVectors(getAvoidanceVector(), getLightVector()))
        -- If the robot is outside the nest
        else
            --look for food and avoid obstacles
            setWheelSpeedsFromVector(addVectors(getAvoidanceVector(), food_vector1))
        end
    end

end

function returning()

    getFoodVector()
    -- If the robot has deposited its food item at the nest
    if not robot.has_food then
        -- Set LEDs to green, and transition to foraging state
        --robot.leds.set_all_colors("green")
        robot.state = "foraging"
    -- If the robot has a food item
    else
        -- Perform phototaxis, while avoiding obstacles
        setWheelSpeedsFromVector(addVectors(getAvoidanceVector(), getLightVector()))
    end

end

function marking()
	local better_leader = 0
	for i = 1, #robot.range_and_bearing do
			if robot.range_and_bearing[i].data[2] > best_value_have_known then
				better_leader = i
			end
	end

	
	if better_leader > 0 then
		--robot.wheels.set_velocity(MAX_SPEED, MAX_SPEED)
		robot.state = "returning"
		robot.range_and_bearing.set_data(1, 10)
		robot.range_and_bearing.set_data(2, 0)--set the food value
		robot.leds.set_all_colors("green")
	else
	robot.wheels.set_velocity(0, 0)
	end

end

function exploring()
    -- If the robot has picked up a food item
    if robot.has_food then
        -- Set LEDs to red, and transition to returning state
        --robot.leds.set_all_colors("red")
        robot.state = "foraging"
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
	explore_step = explore_step + 1
if 	explore_step > max_explore_step then
robot.state = "foraging"
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


function getFoodVector()

    local food_vector = { x = 0, y = 0 }
    local food_best_value = 0
    local best_target = 0
	local robot_class = 10

        -- pick the best food value
 	for i = 1, #robot.range_and_bearing do
		--the food value is saved in data2
			if robot.range_and_bearing[i].data[2] > food_best_value then
				food_best_value = robot.range_and_bearing[i].data[2]
				best_target = i
				robot_class = robot.range_and_bearing[i].data[1]
			--data1=1 mean it is a leader robot
			elseif robot.range_and_bearing[i].data[2] == food_best_value and robot.range_and_bearing[i].data[1] < robot_class then
							best_target = i
							robot_class = robot.range_and_bearing[i].data[1]
			end
	end
	--if it is a class 1 robot, set it color as orange
	if best_target > 0 then 
		if robot_class == 1 then
			robot.leds.set_all_colors("orange")	
		elseif robot_class == 2 then
		--if it is a class 2 robot, set it color as yellow
		robot.leds.set_all_colors("yellow")
		else--other, set it color as green
		robot.leds.set_all_colors("green")
		end 
 		robot.range_and_bearing.set_data(1, (robot_class+1))
		robot.range_and_bearing.set_data(2, food_best_value)

	end
	if food_best_value > best_value_have_known then 
		best_value_have_known = food_best_value
	end		
	
	if robot_class < 5 then--set the vector
		food_vector = newVectorFromPolarCoordinates(robot.range_and_bearing[best_target].range, robot.range_and_bearing[best_target].horizontal_bearing)
return newVectorFromPolarCoordinates(1, getVectorAngleRadians(food_vector))
else 

return { x = 0, y = 0 }

end

end

function calculate_value_to_nest()

local light_value = 0
    for i = 1, #robot.light do
        if robot.light[i].value > light_value then
		light_value = robot.light[i].value
      end
    end
return light_value

end



function setWheelSpeedsFromVector(vector)

     --robot.vectors = { { x = normaliseVectorLength(vector).x, y = normaliseVectorLength(vector).y, color = "orange" } }

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
