# Configuration Veo 3.1 - Instructions pour Claude Code

## Contexte
- Projet Google Cloud : `media-organizer`
- Facturation : ✅ Activée (Niveau payant 1)
- Problème : La clé API actuelle a été créée avant l'activation du Paid Tier

## Action requise par l'utilisateur

### Créer une nouvelle clé API (2 minutes)

1. Aller sur https://aistudio.google.com/apikey
2. Cliquer sur **"Créer une clé API"**
3. Sélectionner le projet **`media-organizer`**
4. Copier la nouvelle clé générée
5. Remplacer l'ancienne clé dans l'application

## Configuration de l'application

### Variable d'environnement
```env
GEMINI_API_KEY=nouvelle-clé-api-ici
```

### Code Python pour Veo 3.1
```python
from google import genai
from google.genai import types

# Clé API Gemini (Paid Tier sur media-organizer)
GEMINI_API_KEY = "NOUVELLE_CLE_API"

# Client unique pour Imagen ET Veo
client = genai.Client(api_key=GEMINI_API_KEY)


def generate_image(prompt: str, output_path: str):
    """Génère une image avec Imagen"""
    response = client.models.generate_images(
        model="imagen-3.0-generate-002",
        prompt=prompt,
        config=types.GenerateImagesConfig(
            number_of_images=1,
            aspect_ratio="16:9",
        )
    )
    
    for image in response.generated_images:
        with open(output_path, "wb") as f:
            f.write(image.image.image_bytes)
        print(f"✅ Image : {output_path}")
    return output_path


def generate_video(prompt: str, output_path: str):
    """Génère une vidéo avec Veo 3.1"""
    
    print("🎬 Génération vidéo en cours... (2-5 minutes)")
    
    operation = client.models.generate_videos(
        model="veo-3.1-generate-preview",
        prompt=prompt,
        config=types.GenerateVideosConfig(
            aspect_ratio="16:9",
            duration_seconds=8,
            generate_audio=True,
            number_of_videos=1,
        )
    )
    
    result = operation.result()
    
    for video in result.generated_videos:
        with open(output_path, "wb") as f:
            f.write(video.video.video_bytes)
        print(f"✅ Vidéo : {output_path}")
    return output_path


def generate_video_from_image(image_path: str, prompt: str, output_path: str):
    """Génère une vidéo à partir d'une image (image-to-video)"""
    
    with open(image_path, "rb") as f:
        image_bytes = f.read()
    
    image = types.Image(image_bytes=image_bytes)
    
    print("🎬 Génération image-to-video en cours...")
    
    operation = client.models.generate_videos(
        model="veo-3.1-generate-preview",
        prompt=prompt,
        image=image,
        config=types.GenerateVideosConfig(
            aspect_ratio="16:9",
            duration_seconds=8,
            generate_audio=True,
        )
    )
    
    result = operation.result()
    
    for video in result.generated_videos:
        with open(output_path, "wb") as f:
            f.write(video.video.video_bytes)
        print(f"✅ Vidéo : {output_path}")
    return output_path
```

## Modèles disponibles

| Modèle | ID API | Usage |
|--------|--------|-------|
| Veo 3.1 | `veo-3.1-generate-preview` | Vidéo HD + audio natif |
| Veo 3.1 Fast | `veo-3.1-fast-generate-preview` | Vidéo rapide, moins cher |
| Veo 3 | `veo-3.0-generate-preview` | Alternative si 3.1 indisponible |
| Imagen 3 | `imagen-3.0-generate-002` | Génération d'images |

## Tarifs (Paid Tier)

| Modèle | Prix |
|--------|------|
| Veo 3.1 Standard | ~$0.40/seconde |
| Veo 3.1 Fast | ~$0.15/seconde |
| Imagen 3 | ~$0.02-0.04/image |

**Exemple** : Vidéo 8 secondes avec audio ≈ $3.20

## Checklist

- [ ] Créer nouvelle clé API sur `media-organizer` dans AI Studio
- [ ] Remplacer l'ancienne clé dans l'app
- [ ] Tester la génération vidéo

## Dépannage

### "Paid API Key Required for Veo"
→ La clé API n'est pas sur le Paid Tier. Créer une nouvelle clé après avoir activé le Paid Tier.

### "Model not found"
→ Essayer `veo-3.0-generate-preview` au lieu de `veo-3.1-generate-preview`

### Génération très longue
→ Normal, Veo prend 2-5 minutes par vidéo de 8 secondes.

### Erreur de quota
→ Vérifier les limites dans AI Studio > Utilisation et facturation > Limite de débit
