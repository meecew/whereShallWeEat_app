import requests
import json
import sys
import time

def fetch_bc_foodies():
    # Overpass API endpoint
    url = "http://overpass-api.de/api/interpreter"

    # Updated headers to satisfy the server's security/integrity checks
    headers = {
        'User-Agent': 'KyleFoodieFetcher/1.0 (kyle-contact-placeholder)', # Identify your script
        'Accept': 'application/json',
        'Content-Type': 'application/x-www-form-urlencoded'
    }

    # Query: British Columbia is large, so we keep the timeout high.
    query = """
    [out:json][timeout:180];
    area["name"="British Columbia"]["admin_level"="4"]->.searchArea;
    (
      nwr["amenity"~"^(restaurant|cafe|fast_food|food_court|pub|ice_cream|canteen|bistro)$"](area.searchArea);
    );
    out center;
    """

    print("--- BC Foodie Fetcher ---")
    print("Sending request to Overpass API...")
    print("Including proper headers to avoid 406 error.")
    print("Searching the entire province. Please wait (1-3 minutes)...")

    start_time = time.time()

    try:
        # We use a POST request with the 'data' parameter
        response = requests.post(
            url,
            data={'data': query},
            headers=headers,
            timeout=300
        )

        print(f"Server responded with status code: {response.status_code}")

        if response.status_code == 200:
            print("Processing data...")
            data = response.json()

            elements = data.get('elements', [])
            total_found = len(elements)
            print(f"Success! Received {total_found} locations.")

            processed_data = []
            for elem in elements:
                tags = elem.get('tags', {})
                amenity_type = tags.get('amenity', 'unknown')
                name = tags.get('name', f"Unnamed {amenity_type}")

                lat = elem.get('lat') or elem.get('center', {}).get('lat')
                lon = elem.get('lon') or elem.get('center', {}).get('lon')

                if lat and lon:
                    processed_data.append({
                        'name': name,
                        'type': amenity_type,
                        'lat': lat,
                        'lon': lon
                    })

            filename = "bc_foodies_database.json"
            with open(filename, "w", encoding='utf-8') as f:
                json.dump(processed_data, f, indent=4, ensure_ascii=False)

            duration = time.time() - start_time
            print(f"\nFINISHED!")
            print(f"Total places saved: {len(processed_data)}")
            print(f"File saved as: {filename}")
            print(f"Time taken: {duration:.1f} seconds")

        elif response.status_code == 429:
            print("\nERROR: Rate limit exceeded (429). The server is busy. Wait 60s.")
        elif response.status_code == 406:
            print("\nERROR: 406 Not Acceptable. The server rejected the request headers.")
            print("Try changing the User-Agent in the script.")
        else:
            print(f"\nERROR: Server returned {response.status_code}")
            print(response.text[:1000])

    except requests.exceptions.Timeout:
        print("\nERROR: Request timed out. The server took too long.")
    except Exception as e:
        print(f"\nAN UNEXPECTED ERROR OCCURRED: {e}")

if __name__ == "__main__":
    sys.stdout.reconfigure(line_buffering=True)
    fetch_bc_foodies()
