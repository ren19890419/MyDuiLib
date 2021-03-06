local function makeKey(defines)
	local result = {}
	for _, v in ipairs(defines) do
		result[v] = v
	end
	return result
end

local events = makeKey
{	
	"BeginPlaying",
	"PlayingPosChanged",
	"PlayingEndReached",
}
return events