"""Bounded offline conversion of the exact pilot launch tones to package PCM."""
from array import array
from io import BytesIO
from pathlib import Path
from subprocess import CalledProcessError, TimeoutExpired, run
from tempfile import TemporaryDirectory, TemporaryFile
import struct
import wave


RATE = 48000


def launch_pcm(wav, *, max_seconds=3):
    if max_seconds not in (3,4):
        raise ValueError("unsupported PCM duration limit")
    try:
        with wave.open(BytesIO(wav)) as source:
            n,rate = source.getnframes(),source.getframerate()
            if source.getnchannels()!=1 or source.getsampwidth()!=2 or not 8000<=rate<=192000 or not n:
                raise ValueError("launch WAV format")
            frames = (n*RATE+rate-1)//rate
            if frames>RATE*max_seconds:
                raise ValueError("launch PCM frame budget")
            raw = source.readframes(n)
    except (EOFError,wave.Error) as exc:
        raise ValueError("invalid launch WAV") from exc
    if len(raw)!=n*2:
        raise ValueError("short launch WAV samples")
    mono = struct.unpack(f"={n}h",raw) # wave delivers native-endian 16-bit samples
    stereo = array("h")
    for frame in range(frames):
        index,fraction = divmod(frame*rate,RATE)
        weight = mono[index]*(RATE-fraction)+mono[min(index+1,n-1)]*fraction
        value = (abs(weight)//RATE)*(1 if weight>=0 else -1)
        stereo.extend((value,value))
    if struct.pack("=H",1)!=b"\x01\x00":
        stereo.byteswap()
    return stereo.tobytes()


def decode_shot_audio(decoder, tones, files, bank_members, *, max_seconds=3):
    decoder = Path(decoder).resolve(strict=True)
    if not decoder.is_file() or not 1<=len(tones)<=16 or max_seconds not in (3,4):
        raise ValueError("invalid launch decoder or tone set")
    result = {}
    with TemporaryDirectory(prefix="gf-roster-launch-") as temp:
        staged = {}
        for bank,tone in sorted(tones):
            if bank not in staged:
                paths = []
                for ext in ("tsb","chd","dpk"):
                    path = Path(temp)/f"{bank}.{ext}"
                    with path.open("xb") as dest:
                        dest.write(files[bank_members[bank][ext]["path"]])
                    paths.append(path)
                staged[bank] = paths
            try:
                with TemporaryFile(dir=temp) as wav:
                    run([decoder,*staged[bank],str(tone)],stdout=wav,check=True,timeout=10)
                    wav.seek(0)
                    limit=44+192000*max_seconds*2
                    data = wav.read(limit+1)
            except (CalledProcessError,TimeoutExpired) as exc:
                raise ValueError(f"launch sample refused: {bank}/{tone}") from exc
            if len(data)>limit:
                raise ValueError("launch WAV byte budget")
            result[bank,tone] = launch_pcm(data,max_seconds=max_seconds)
    return result
