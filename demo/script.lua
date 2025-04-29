x = True
function loop()
	if im.beginWindow("hello", 300, 100) ~= 0 then
		if im.button("Hello") ~= 0 then
			x = not x
		end
		if x then
			im.label("Hello")
		end
		im.endWindow()
	end
end
