export const REPOSITORY='nicolenkoe/screen-recorder-downloads';
export const RELEASES_URL='https://github.com/'+REPOSITORY+'/releases';
export const API_URL='https://api.github.com/repos/'+REPOSITORY+'/releases';
export function channel(release){
 const label=(release.tag_name+' '+(release.name||'')).toLowerCase();
 if(/experimental|эксперимент/.test(label))return {id:'experimental',label:'Экспериментальная'};
 if(release.prerelease||/(?:^|[-_. ])(?:alpha|beta|test|testing|rc\d*)(?:$|[-_. \d])|тестов/.test(label))return {id:'test',label:'Тестовая'};
 return {id:'stable',label:'Стабильная'};
}
function assetUrl(asset){
 try{const u=new URL(asset.browser_download_url);return u.protocol==='https:'&&u.host==='github.com'&&u.pathname.startsWith('/'+REPOSITORY+'/releases/download/')?u.href:null;}catch{return null;}
}
export function listBuilds(releases){
 const builds=[];
 for(const r of releases){
  const published=Date.parse(r.published_at);
  if(r.draft||!Number.isFinite(published))continue;
  for(const platform of ['android','windows']){
   const extension=platform==='android'?/^ScreenRecorder.*\.apk$/i:/^ScreenRecorder.*\.exe$/i;
   const assets=(Array.isArray(r.assets)?r.assets:[]).filter(a=>extension.test(a.name)&&a.size>0&&(!a.state||a.state==='uploaded')&&assetUrl(a));
   assets.sort((a,b)=>Number(/^(ScreenRecorder\.apk|ScreenRecorder-Windows-[\d.]+\.exe)$/i.test(b.name))-Number(/^(ScreenRecorder\.apk|ScreenRecorder-Windows-[\d.]+\.exe)$/i.test(a.name))||a.name.localeCompare(b.name));
   if(assets.length)builds.push({platform,tag:r.tag_name,name:r.name||r.tag_name,published,id:Number(r.id)||0,channel:channel(r),asset:assets[0],url:assetUrl(assets[0])});
  }
 }
 return builds.sort((a,b)=>b.published-a.published||b.id-a.id||a.tag.localeCompare(b.tag));
}
export function latestBuild(releases,platform){return listBuilds(releases).find(r=>r.platform===platform)||null;}
export async function fetchReleases(fetcher=fetch){
 const all=[];
 for(let page=1;;page++){
  const abort=new AbortController(),timer=setTimeout(()=>abort.abort(),15000);
  try{
   const r=await fetcher(API_URL+'?per_page=100&page='+page,{cache:'no-store',headers:{Accept:'application/vnd.github+json'},signal:abort.signal});
   if(!r.ok)throw new Error('GitHub: '+r.status);
   const rows=await r.json();if(!Array.isArray(rows))throw new Error('Некорректный каталог GitHub');
   all.push(...rows);if(rows.length<100)return all;
  }finally{clearTimeout(timer);}
 }
}
export function fileSize(bytes){return new Intl.NumberFormat('ru-RU',{maximumFractionDigits:1}).format(bytes/(bytes<1048576?1024:1048576))+(bytes<1048576?' КБ':' МБ');}
export function releaseDate(timestamp){return new Intl.DateTimeFormat('ru-RU',{day:'numeric',month:'long',year:'numeric'}).format(timestamp);}
