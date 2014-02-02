import pygame

def audio_widget(r,tb,height=100):
	data=r.get_chunks()

	audio=pygame.Surface((r.CAPTURE_CHUNKS,height))

	colors=[(0,0,255),(100,100,255),(200,200,255)]
	beatline_color=(255,0,0)
	for j in range(len(r.RANGES)):
		scaling_factor=max([d[0][j] for d in data])
		if scaling_factor<1: scaling_factor=1
		for i in range(r.CAPTURE_CHUNKS):
			beatline=data[i][1]
			if not beatline:
				val=float(data[i][0][j])/scaling_factor
				pygame.draw.line(audio,colors[j],(i,(height-height*val)/2),(i,(height+height*val)/2))
			else:
				pygame.draw.line(audio,beatline_color,(i,0),(i,height))

	scaler=1.-tb.get_fractick()
	bg_color=(255,255,255)
	beat_c=[int(c*scaler) for c in bg_color]
	right=r.CAPTURE_CHUNKS-1
	pygame.draw.line(audio,beat_c,(right,0),(right,height))

	return audio

def pattern_timeline_widget(tb,seq,seconds_per_pixel,height=100,width=400):
	timeline=pygame.Surface((width,height))
	seconds_per_beat=tb.period
	pixels_per_beat=seconds_per_beat/seconds_per_pixel
	now=tb.get_tick()
	occupied_space=[]
	patterns=seq.get_patterns()

	def overlaps(a,b,c,d):
		if b is None and d is None:
			return True
		if b is None:
			return d>a
		if d is None:
			return b>c
		return (c<a and a<d) or (c<b and b<d) or (a<c and c<b) or (a<d and d<b) or a == c or b == d

	def render_pattern(surface,pat,start,start_px,stop_px,slot):
		pygame.draw.rect(surface,(50,50,50),((start_px,1+slot*20),(stop_px-start_px-2,18)))

		for start,duration,effect in pat.get_events((start_px-(start-now)*pixels_per_beat)/pixels_per_beat,(stop_px-(start-now)*pixels_per_beat)/pixels_per_beat):
			if effect.render is None:
				continue
			eff_start_px=start_px+start*pixels_per_beat
			eff_stop_px=eff_start_px+duration*pixels_per_beat
			if eff_stop_px <= start_px or eff_start_px >= stop_px:
				continue
			effect.render(surface,int(eff_start_px),int(eff_stop_px),int(start_px),int(stop_px),slot)

		font = pygame.font.Font(None, 16)
		text = font.render(pat.name, 1, (255,255,255))
		x=min(max(5,start_px+5),stop_px-text.get_width()-8)
		surface.blit(text,(x,2+slot*20))

		pygame.draw.rect(surface,(255,255,255),((start_px,1+slot*20),(stop_px-start_px-2,18)),1)

	for start,stop,p in patterns:
		slot=-1
		for i in range(len(occupied_space)):
			wontfit=False
			for o_start,o_stop in occupied_space[i]:
				if overlaps(start,stop,o_start,o_stop):
					wontfit=True
					break
			if not wontfit:
				occupied_space[i].append((start,stop))
				slot=i
				break
		if slot<0:
			slot=len(occupied_space)
			occupied_space.append([(start,stop)])

		start_px=(start-now)*pixels_per_beat
		if stop is None:
			stop_px=width
		else:
			stop_px=(stop-now)*pixels_per_beat

		render_pattern(timeline,p,start,start_px,stop_px,slot)

	return timeline

def full_color(color,surface,eff_start_px,eff_stop_px,start_px,stop_px,slot):
	start=max(eff_start_px,start_px)
	stop=min(eff_stop_px,stop_px)
	pygame.draw.rect(surface,color,((start,1+slot*20),(stop-start-2,18)))

def strobe(color,surface,eff_start_px,eff_stop_px,start_px,stop_px,slot):
	pygame.draw.circle(surface,color,(eff_start_px,10+slot*20),2)

def strip2screen((r,g,b,a)):
	return (r,min(g*3,255),min(b*3,255))
