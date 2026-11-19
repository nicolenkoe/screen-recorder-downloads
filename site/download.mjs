import {fetchReleases,latestBuild,RELEASES_URL,fileSize} from './release-data.mjs';
const platform=new URLSearchParams(location.search).get('platform');
const title=document.querySelector('#download-title'),status=document.querySelector('#download-status'),link=document.querySelector('#direct-download'),retry=document.querySelector('#retry-download');
document.querySelector('#github-releases').href=RELEASES_URL;
let busy=false;
async function resolve(){
 if(busy)return;
 if(!['android','windows'].includes(platform)){title.textContent='Выберите платформу';status.textContent='На главной странице есть отдельные кнопки для Android и Windows.';retry.hidden=true;return;}
 busy=true;retry.hidden=true;link.hidden=true;title.textContent='Скачать для '+(platform==='android'?'Android':'Windows');status.textContent='Ищем самую новую сборку на GitHub…';
 try{
  const build=latestBuild(await fetchReleases(),platform);
  if(!build)throw new Error('Сборки пока не опубликованы');
  link.href=build.url;link.textContent='↓ Скачать '+build.tag;link.hidden=false;
  status.textContent=build.channel.label+' · '+fileSize(build.asset.size)+'. Начинаем скачивание. Если оно не началось, нажмите кнопку ниже.';
  location.replace(build.url);
 }catch(error){status.textContent='Не удалось получить последнюю сборку. Проверьте соединение и повторите попытку или откройте выпуски на GitHub.';retry.hidden=false;}
 finally{busy=false;}
}
retry.addEventListener('click',resolve);
resolve();
