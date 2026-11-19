
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

// The main download links always resolve again at click time on download.html.
import('./release-data.mjs').then(async({fetchReleases,latestBuild,fileSize})=>{
 const releases=await fetchReleases();
 for(const platform of ['android','windows']){
  const build=latestBuild(releases,platform);if(!build)continue;
  document.querySelector('#channel-'+platform).textContent=build.channel.label;
  document.querySelector('#latest-'+platform).textContent=build.tag+' · '+fileSize(build.asset.size);
 }
}).catch(()=>{}); // Keep the neutral labels and working resolver links if GitHub is unavailable.
