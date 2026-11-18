const fontButton=document.querySelector('#font');const fontMenu=document.querySelector('#font-menu');let scale=100;try{scale=Number(localStorage.getItem('font')||100);}catch{}function setFont(value){if(![100,125,150,175,200].includes(value))value=100;document.documentElement.style.fontSize=`${18*value/100}px`;document.querySelectorAll('[data-font]').forEach(b=>b.setAttribute('aria-pressed',String(Number(b.dataset.font)===value)));try{localStorage.setItem('font',value);}catch{}}setFont(scale);fontButton.addEventListener('click',()=>{fontMenu.hidden=!fontMenu.hidden;fontButton.setAttribute('aria-expanded',String(!fontMenu.hidden));});document.querySelectorAll('[data-font]').forEach(b=>b.addEventListener('click',()=>{setFont(Number(b.dataset.font));fontMenu.hidden=true;fontButton.setAttribute('aria-expanded','false');fontButton.focus();}));document.addEventListener('keydown',e=>{if(e.key==='Escape'){fontMenu.hidden=true;fontButton.setAttribute('aria-expanded','false');}});document.addEventListener('click',e=>{if(!fontMenu.contains(e.target)&&e.target!==fontButton){fontMenu.hidden=true;fontButton.setAttribute('aria-expanded','false');}});

// Load a versioned media file so browsers cannot reuse the previous lesson.
fetch('tutorial.json?v=302-20261119',{cache:'no-store'}).then(r=>r.ok?r.json():null).then(data=>{
 if(!data||!data.url)return;
 const video=document.querySelector('#tutorial'),poster=document.querySelector('#video-poster'),play=document.querySelector('.play'),status=document.querySelector('#video-status');
 const url=new URL(data.url,location.href);
 if(url.origin!==location.origin&&url.protocol!=='https:')return;
 video.src=url.href;play.disabled=false;play.setAttribute('aria-label','Смотреть видеоинструкцию');
 const seconds=Math.round(Number(data.durationSeconds)||0);
 poster.querySelector('h3').textContent='Запись и обрезка — шаг за шагом';
 poster.querySelector('.badge').textContent=Math.floor(seconds/60)+' мин '+String(seconds%60).padStart(2,'0')+' с · женская озвучка';
 status.textContent='Нажмите ▶, чтобы начать просмотр со звуком.';
 video.addEventListener('waiting',()=>{status.textContent='Загрузка видео…';});
 video.addEventListener('playing',()=>{status.textContent='Видео со звуком. Субтитры включаются кнопкой в плеере.';});
 video.addEventListener('error',()=>{video.hidden=true;poster.hidden=false;status.textContent='Не удалось загрузить ролик. Повторите запуск или откройте MP4 по ссылке ниже.';});
 const start=()=>{
  document.querySelector('#guide').scrollIntoView({behavior:matchMedia('(prefers-reduced-motion: reduce)').matches?'auto':'smooth'});
  poster.hidden=true;video.hidden=false;
  video.play().catch(()=>{video.controls=true;status.textContent='Нажмите кнопку воспроизведения в плеере, чтобы начать просмотр.';video.focus();});
 };
 play.addEventListener('click',start);
 document.querySelectorAll('a[href="#guide"]').forEach(a=>a.addEventListener('click',start));
}).catch(()=>{document.querySelector('#video-status').textContent='Проверьте соединение или откройте MP4 по ссылке ниже.';});
