import {fetchReleases,listBuilds,fileSize,releaseDate,RELEASES_URL} from './release-data.mjs';
const list=document.querySelector('#version-list'),status=document.querySelector('#versions-status'),retry=document.querySelector('#retry-versions'),filter=document.querySelector('#platform-filter');
let builds=[];
function render(){
 list.replaceChildren();
 const shown=builds.filter(b=>filter.value==='all'||b.platform===filter.value);
 status.textContent=shown.length?'Опубликованных сборок: '+shown.length+'. Новые выпуски показаны первыми.':'Сборок для выбранной платформы пока нет.';
 for(const build of shown){
  const card=document.createElement('article');card.className='version-card';
  const top=document.createElement('div');top.className='version-top';
  const platform=document.createElement('span');platform.textContent=build.platform==='android'?'Android':'Windows';
  const badge=document.createElement('span');badge.className='badge channel-'+build.channel.id;badge.textContent=build.channel.label;
  top.append(platform,badge);
  const title=document.createElement('h2');title.textContent=build.tag;
  const metadata=document.createElement('p');metadata.className='subtle';metadata.textContent=releaseDate(build.published)+' · '+fileSize(build.asset.size);
  const download=document.createElement('a');download.className='button primary';download.href=build.url;download.textContent='↓ Скачать эту версию';
  const details=document.createElement('a');details.className='release-details';details.href=RELEASES_URL+'/tag/'+encodeURIComponent(build.tag);details.textContent='Описание выпуска на GitHub';details.target='_blank';details.rel='noopener';
  card.append(top,title,metadata,download,details);list.append(card);
 }
}
async function load(){
 retry.hidden=true;status.textContent='Загружаем список версий с GitHub…';
 try{builds=listBuilds(await fetchReleases());render();}
 catch{status.textContent='Не удалось загрузить версии. Проверьте соединение и повторите попытку.';retry.hidden=false;}
}
filter.addEventListener('change',render);retry.addEventListener('click',load);load();
